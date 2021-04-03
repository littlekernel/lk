// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/unique_fd.h>

#include <fbl/algorithm.h>
#include <fbl/type_support.h>
#include <unittest/unittest.h>

namespace {

bool invalid_fd_test() {
    BEGIN_TEST;

    {
        fbl::unique_fd fd;

        EXPECT_EQ(fd.get(), fbl::unique_fd::InvalidValue());
        EXPECT_EQ(fbl::unique_fd::InvalidValue(), fd.get());

        EXPECT_EQ(static_cast<int>(fd), fbl::unique_fd::InvalidValue());
        EXPECT_EQ(fbl::unique_fd::InvalidValue(), static_cast<int>(fd));

        EXPECT_EQ(false, fd.is_valid());
        EXPECT_EQ(static_cast<bool>(fd), false);
        EXPECT_EQ(false, static_cast<bool>(fd));

        EXPECT_FALSE(fd);
    }

    END_TEST;
}

bool valid_comparison_test() {
    BEGIN_TEST;

    int pipes[2];
    EXPECT_EQ(pipe(pipes), 0);
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        EXPECT_NE(in.get(), fbl::unique_fd::InvalidValue());
        EXPECT_NE(out.get(), fbl::unique_fd::InvalidValue());
        EXPECT_NE(fbl::unique_fd::InvalidValue(), in.get());
        EXPECT_NE(fbl::unique_fd::InvalidValue(), out.get());

        EXPECT_EQ(in.get(), in.get());
        EXPECT_NE(in.get(), out.get());
        EXPECT_FALSE(in == out);
        EXPECT_TRUE(in == in);
        EXPECT_TRUE(out == out);
        EXPECT_EQ(pipes[1], in.get());

        EXPECT_TRUE(in);
        EXPECT_TRUE(out);
    }

    END_TEST;
}

bool verify_pipes_open(int in, int out) {
    BEGIN_HELPER;
    char w = 'a';
    EXPECT_EQ(write(in, &w, 1), 1);
    char r;
    EXPECT_EQ(read(out, &r, 1), 1);
    EXPECT_EQ(r, w);
    END_HELPER;
}

bool verify_pipes_closed(int in, int out) {
    BEGIN_HELPER;
    char c = 'a';
    EXPECT_EQ(write(in, &c, 1), -1);
    EXPECT_EQ(read(out, &c, 1), -1);
    END_HELPER;
}

bool scoping_test() {
    BEGIN_TEST;
    int pipes[2];
    EXPECT_EQ(pipe(pipes), 0);
    EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        EXPECT_EQ(pipes[0], out.get());
        EXPECT_EQ(pipes[1], in.get());
        EXPECT_TRUE(verify_pipes_open(in.get(), out.get()));
    }
    EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
    END_TEST;
}

bool swap_test() {
    BEGIN_TEST;
    int pipes[2];
    EXPECT_EQ(pipe(pipes), 0);
    EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        in.swap(out);
        EXPECT_EQ(pipes[0], in.get());
        EXPECT_EQ(pipes[1], out.get());
        EXPECT_TRUE(verify_pipes_open(out.get(), in.get()));
    }
    EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
    EXPECT_TRUE(verify_pipes_closed(pipes[0], pipes[1]));
    END_TEST;
}

bool move_test() {
    BEGIN_TEST;
    // Move assignment
    int pipes[2];
    EXPECT_EQ(pipe(pipes), 0);
    EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        fbl::unique_fd in2, out2;
        EXPECT_TRUE(verify_pipes_open(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_closed(in2.get(), out2.get()));

        in2 = fbl::move(in);
        out2 = fbl::move(out);

        EXPECT_TRUE(verify_pipes_closed(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_open(in2.get(), out2.get()));
    }
    EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));

    // Move constructor
    EXPECT_EQ(pipe(pipes), 0);
    EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        EXPECT_TRUE(verify_pipes_open(in.get(), out.get()));

        fbl::unique_fd in2 = fbl::move(in);
        fbl::unique_fd out2 = fbl::move(out);

        EXPECT_TRUE(verify_pipes_closed(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_open(in2.get(), out2.get()));
    }
    EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
    END_TEST;
}

bool reset_test() {
    BEGIN_TEST;
    int pipes[2];
    EXPECT_EQ(pipe(pipes), 0);
    int other_pipes[2];
    EXPECT_EQ(pipe(other_pipes), 0);
    EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
    EXPECT_TRUE(verify_pipes_open(other_pipes[1], other_pipes[0]));
    {
        fbl::unique_fd in(pipes[1]);
        fbl::unique_fd out(pipes[0]);

        EXPECT_TRUE(verify_pipes_open(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_open(pipes[1], pipes[0]));
        EXPECT_TRUE(verify_pipes_open(other_pipes[1], other_pipes[0]));

        in.reset(other_pipes[1]);
        out.reset(other_pipes[0]);

        EXPECT_TRUE(verify_pipes_open(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
        EXPECT_TRUE(verify_pipes_open(other_pipes[1], other_pipes[0]));

        in.reset();
        out.reset();

        EXPECT_TRUE(verify_pipes_closed(in.get(), out.get()));
        EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
        EXPECT_TRUE(verify_pipes_closed(other_pipes[1], other_pipes[0]));
    }
    EXPECT_TRUE(verify_pipes_closed(pipes[1], pipes[0]));
    EXPECT_TRUE(verify_pipes_closed(other_pipes[1], other_pipes[0]));
    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(unique_fd_tests)
RUN_TEST(invalid_fd_test)
RUN_TEST(valid_comparison_test)
RUN_TEST(scoping_test)
RUN_TEST(swap_test)
RUN_TEST(move_test)
RUN_TEST(reset_test)
END_TEST_CASE(unique_fd_tests)
