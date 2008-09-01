/*
 * Copyright (c) 2006 Brian Swetland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __HW_MII_H
#define __HW_MII_H

#define MII_REG_BCR           0x00
#define MII_REG_BSR           0x01
#define MII_REG_PHY_ID1       0x02
#define MII_REG_PHY_ID2       0x03
#define MII_REG_AUTO_ADV      0x04
#define MII_REG_AUTO_LINK     0x05
#define MII_REG_AUTO_EXPN     0x06
#define MII_REG_AUTO_NEXT     0x07
#define MII_REG_LINK_NEXT     0x08
#define MII_REG_RXER_CNT      0x15
#define MII_REG_ICSR          0x1b
#define MII_REG_100TX_PHY     0x1f

#define MII_BCR_RESET         0x8000
#define MII_BCR_LOOPBACK      0x4000
#define MII_BCR_100MBPS       0x2000
#define MII_BCR_AUTO_ENABLE   0x1000
#define MII_BCR_PWR_DOWN      0x0800
#define MII_BCR_ISOLATE       0x0400
#define MII_BCR_AUTO_RESTART  0x0200
#define MII_BCR_FULL_DUPLEX   0x0100
#define MII_BCR_COL_TEST      0x0080
#define MII_BCR_TX_DISABLE    0x0001

#define MII_BSR_T4            0x8000
#define MII_BSR_100TX_FULL    0x4000
#define MII_BSR_100TX_HALF    0x2000
#define MII_BSR_10T_FULL      0x1000
#define MII_BSR_10T_HALF      0x0800
#define MII_BSR_NO_PREAMBLE   0x0040
#define MII_BSR_AUTO_COMPLETE 0x0020
#define MII_BSR_REMOTE_FAULT  0x0010
#define MII_BSR_AUTO_ABLE     0x0008
#define MII_BSR_LINK_UP       0x0004
#define MII_BSR_JABBER        0x0002
#define MII_BSR_EXTEND        0x0001

#define MII_100TX_PHY_ISOLATE  0x0040
#define MII_100TX_MODE_MASK    0x001C
#define MII_100TX_MODE_AUTO    0x0000
#define MII_100TX_MODE_10T_H   0x0004
#define MII_100TX_MODE_100TX_H 0x0008
#define MII_100TX_MODE_10T_F   0x0014
#define MII_100TX_MODE_100TX_F 0x0018
#define MII_100TX_MODE_ISOLATE 0x001C
#define MII_100TX_SQE_TEST     0x0002
#define MII_100TX_NO_SCRAMBLE  0x0001

#endif
