// Copyright (c) 2025 Linaro Ltd.
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

//! Rust PL011 uart driver for LK.

#![no_std]

use core::{
    cell::UnsafeCell,
    ffi::{c_char, c_int, c_void},
    mem::offset_of,
    panic,
    ptr::NonNull,
};
use lk::{cbuf::Cbuf, lkonce::LkOnce, sys};
use safe_mmio::{
    UniqueMmioPointer, field,
    fields::{ReadWrite, WriteOnly},
};

extern crate alloc;

unsafe extern "C" {
    static mut console_input_cbuf: sys::cbuf;
}

fn get_console_input_cbuf() -> Cbuf {
    unsafe { Cbuf::new(&raw mut console_input_cbuf) }
}

/// Init function to ensure crate is linked.
/// This might not be needed once we provide something actually linked.
pub fn init() {
    // Nothing to do yet.
}

/// Representation of the UART registers for MMIO access.
#[repr(C)]
pub struct UartRegs {
    dr: ReadWrite<u32>,    // 0x00 Data Register
    rsr: ReadWrite<u32>,   // 0x04 Receive Status / Error Clear
    _reserved1: [u32; 4],  // 0x08 - 0x0C reserved
    fr: ReadWrite<u32>,    // 0x18 Flag Register
    _reserved2: [u32; 1],  // 0x1C reserved
    iplr: ReadWrite<u32>,  // 0x20 Interrupt Priority Level Register
    ibrd: ReadWrite<u32>,  // 0x24 Integer Baud Rate Divisor
    fbrd: ReadWrite<u32>,  // 0x28 Fractional Baud Rate Divisor
    lcrh: ReadWrite<u32>,  // 0x2C Line Control Register
    cr: ReadWrite<u32>,    // 0x30 Control Register
    ifls: ReadWrite<u32>,  // 0x34 Interrupt FIFO Level Select
    imsc: ReadWrite<u32>,  // 0x38 Interrupt Mask Set/Clear
    ris: ReadWrite<u32>,   // 0x3C Raw Interrupt Status
    mis: ReadWrite<u32>,   // 0x40 Masked Interrupt Status
    icr: WriteOnly<u32>,   // 0x44 Interrupt Clear Register
    dmacr: ReadWrite<u32>, // 0x48 DMA Control Register
}

/// Verify that UartRegs has the expected layout
#[allow(dead_code)]
fn verify_uart_regs_layout() {
    // These will cause compile-time errors if offsets don't match
    const _: () = assert!(offset_of!(UartRegs, dr) == 0x00);
    const _: () = assert!(offset_of!(UartRegs, rsr) == 0x04);
    const _: () = assert!(offset_of!(UartRegs, fr) == 0x18);
    const _: () = assert!(offset_of!(UartRegs, iplr) == 0x20);
    const _: () = assert!(offset_of!(UartRegs, ibrd) == 0x24);
    const _: () = assert!(offset_of!(UartRegs, fbrd) == 0x28);
    const _: () = assert!(offset_of!(UartRegs, lcrh) == 0x2c);
    const _: () = assert!(offset_of!(UartRegs, cr) == 0x30);
    const _: () = assert!(offset_of!(UartRegs, ifls) == 0x34);
    const _: () = assert!(offset_of!(UartRegs, imsc) == 0x38);
    const _: () = assert!(offset_of!(UartRegs, ris) == 0x3c);
    const _: () = assert!(offset_of!(UartRegs, mis) == 0x40);
    const _: () = assert!(offset_of!(UartRegs, icr) == 0x44);
    const _: () = assert!(offset_of!(UartRegs, dmacr) == 0x48);
}

const CONSOLE_HAS_INPUT_BUFFER: bool = true;
const PL011_FLAG_DEBUG_UART: u32 = 0x1;

const UART_TFR_RXFE: u32 = 1 << 4; // UART Receive FIFO Empty
const UART_TFR_TXFF: u32 = 1 << 5; // UART Transmit FIFO Full
// const UART_TFR_RXFF: u32 = 1 << 6; // UART Receive FIFO Full
// const UART_TFR_TXFE: u32 = 1 << 7; // UART Transmit FIFO Empty

const UART_IMSC_RXIM: u32 = 1 << 4; // Receive interrupt mask

const UART_TMIS_RXMIS: u32 = 1 << 4; // Receive masked interrupt status

const UART_CR_RXEN: u32 = 1 << 9; // Receive enable

// TODO: Don't make this manually.
#[repr(C)]
#[derive(Clone)]
#[allow(non_camel_case_types)]
struct pl011_config {
    base: usize,
    irq: u32,
    flag: u32,
}

/// Global state of the uart. Very _non_ rust friendly right now, just a
/// directly translation from the C code.
struct Pl011Uart {
    config: pl011_config,
    rx_buf: UnsafeCell<sys::cbuf>,
}

/// The underlying cbuf is actually Sync because it uses its own spinlock.
unsafe impl Sync for Pl011Uart {}

/// Get a reference to the given uart.
fn get_uart(port: usize) -> &'static Pl011Uart {
    if port >= PL011_UARTS.len() {
        // For now, just panic if we access an invalid port.
        panic!("pl011: invalid port {}", port);
    }
    PL011_UARTS[port].get()
}

/// Get a reference to the given uart that only requires early init to have been done.
fn get_uart_early(port: usize) -> &'static Pl011Uart {
    if port >= PL011_UARTS.len() {
        // For now, just panic if we access an invalid port.
        panic!("pl011: invalid port {}", port);
    }
    PL011_UARTS[port].get_early()
}

impl Pl011Uart {
    /// Get the mmio handle for the given UART.
    ///
    /// # Safety
    /// This routine is not marked as unsafe, as we assume that the struct has
    /// been initialized with a proper offset of the base of the uart registers.
    /// Although the docs for safe-mmio suggest that having multiple instances
    /// of this handle is unsafe, this is not a rust safety issue. The hardware
    /// block, and memory semantics are well defined for multiple concurrent
    /// access (including writes) and incorrect use is a logic error in the
    /// driver, not an issue of Rust memory safety.
    ///
    /// But, this conflation of the Rust borrow checker's rules around only one
    /// instance of a mutable reference, and how the hardware works is
    /// persistent. As the underlying MMIO is implemented using raw pointers,
    /// there are no references, having multiple instances of this handle is
    /// perfectly safe.
    ///
    /// It is important that the returned handle is 'static, otherwise we only
    /// get one borrow which defeats the entire purpose of this routine.
    fn get_dev(&self) -> UniqueMmioPointer<'static, UartRegs> {
        unsafe { UniqueMmioPointer::new(NonNull::new(self.config.base as _).unwrap()) }
    }
}

const RXBUF_SIZE: usize = 32;

static PL011_UARTS: [LkOnce<Pl011Uart>; 1] = [LkOnce::uninit()];

extern "C" fn uart_irq(arg: *mut c_void) -> sys::handler_return {
    let uart = unsafe { &*(arg as *const Pl011Uart) };
    let mut dev = uart.get_dev();
    let buf = get_console_input_cbuf();

    // uart_pputc(0, b'I');

    let mut resched = false;

    let isr = field!(dev, mis).read();

    // Read characters from the fifo until it's empty.
    // TODO: This is wrong, and doesn't handle the cbuf overflow.
    if (isr & UART_TMIS_RXMIS) != 0 {
        while (field!(dev, fr).read() & UART_TFR_RXFE) == 0 {
            if CONSOLE_HAS_INPUT_BUFFER {
                if uart.config.flag & PL011_FLAG_DEBUG_UART != 0 {
                    let c = field!(dev, dr).read() as c_char;
                    buf.write_char(c, false);
                    continue;
                }
            }

            // If we're out of rx buffer, mask the irq instead of handling it.
            // TODO: This is racy, as the interrupt is owned by the user side code.
            if buf.is_full() {
                let old = field!(dev, imsc).read();
                field!(dev, imsc).write(old & !UART_IMSC_RXIM);
                break;
            }
            let c = field!(dev, dr).read() as c_char;
            buf.write_char(c, false);
        }

        resched = true;
    }

    if resched {
        sys::INT_RESCHEDULE
    } else {
        sys::INT_NO_RESCHEDULE
    }
}

/// Early initialization sets the config so that panic print messages can output
/// characters.
#[unsafe(no_mangle)]
extern "C" fn pl011_init_early(port: isize, config: *const pl011_config) {
    PL011_UARTS[port as usize].early_init(|uart| unsafe {
        (*uart).config = (*config).clone();
    });
}

#[unsafe(no_mangle)]
extern "C" fn pl011_init(port: isize) {
    unsafe {
        PL011_UARTS[port as usize].init(|uart| {
            let uart = &mut *uart;
            sys::cbuf_initialize(uart.rx_buf.get(), RXBUF_SIZE);

            let mut dev = uart.get_dev();

            sys::register_int_handler(
                uart.config.irq,
                Some(uart_irq),
                uart as *mut Pl011Uart as *mut c_void,
            );

            // Clear all irqs
            field!(dev, icr).write(0x3FF);

            // Set fifo trigger level
            field!(dev, ifls).write(0); // 1/8 full, 1/8 txfifo

            // Enable rx interrupt
            let old = field!(dev, imsc).read();
            field!(dev, imsc).write(old | UART_IMSC_RXIM);

            // Enable receive
            let old = field!(dev, cr).read();
            field!(dev, cr).write(old | UART_CR_RXEN);

            // Enable interrupt
            sys::unmask_interrupt(uart.config.irq);
        })
    };
}

#[unsafe(no_mangle)]
extern "C" fn uart_putc(port: c_int, c: c_char) {
    let uart = get_uart_early(port as usize);
    let mut dev = uart.get_dev();

    // Spin while fifo is full.
    while (field!(dev, fr).read() & UART_TFR_TXFF) != 0 {}
    field!(dev, dr).write(c as u32);
}

#[unsafe(no_mangle)]
extern "C" fn uart_getc(port: c_int, wait: bool) -> c_int {
    let uart = get_uart(port as usize);
    let mut dev = uart.get_dev();
    let buf = unsafe { Cbuf::new(uart.rx_buf.get()) };

    if let Some(ch) = buf.read_char(wait) {
        // TODO: This modify is racy, as the interrupt handler also does a
        // modify of this same register. As long as we are only using a single
        // interrupt, this isn't unsafe or incorrect, but will cause problems if
        // we even implement the transmit interrupt as well.
        // Realistically, the interrupt handler should not be touching this register
        let mut f = field!(dev, mis);
        let old = f.read();
        f.write(old | UART_TMIS_RXMIS);
        return ch as c_int;
    }

    -1
}

/// Panic-time putc.
#[unsafe(no_mangle)]
extern "C" fn uart_pputc(port: c_int, c: c_char) -> c_int {
    let uart = get_uart_early(port as usize);
    let mut dev = uart.get_dev();

    // Spin while fifo is full.
    while field!(dev, fr).read() & UART_TFR_TXFF != 0 {}
    field!(dev, dr).write(c as u32);

    return 1;
}

#[unsafe(no_mangle)]
extern "C" fn uart_pgetc(port: c_int) {
    let _ = port;

    panic!("uart_pgetc not implemented");
}
