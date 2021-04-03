// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unittest/unittest.h>
#include <fbl/intrusive_wavl_tree.h>

namespace fbl {
namespace tests {
namespace intrusive_containers {

using ::fbl::internal::is_sentinel_ptr;
using ::fbl::internal::valid_sentinel_ptr;

class WAVLTreeChecker {
public:
    template <typename TreeType>
    static bool VerifyParentBackLinks(const TreeType& tree,
                                      typename TreeType::RawPtrType node) {
        BEGIN_TEST;
        using NodeTraits = typename TreeType::NodeTraits;

        ASSERT_TRUE(valid_sentinel_ptr(node), "");
        const auto& ns = NodeTraits::node_state(*node);

        if (valid_sentinel_ptr(ns.left_)) {
            EXPECT_EQ(node, NodeTraits::node_state(*ns.left_).parent_,
                      "Corrupt left-side parent back-link!");
        }

        if (valid_sentinel_ptr(ns.right_)) {
            EXPECT_EQ(node, NodeTraits::node_state(*ns.right_).parent_,
                      "Corrupt right-side parent back-link!");
        }

        END_TEST;
    }

    template <typename TreeType>
    static bool SanityCheck(const TreeType& tree) {
        BEGIN_TEST;
        using NodeTraits = typename TreeType::NodeTraits;
        using RawPtrType = typename TreeType::RawPtrType;
        using Observer   = typename TreeType::Observer;

        // Enforce the sentinel requirements.
        if (tree.is_empty()) {
            // If the tree is empty, then the root should be null, and the
            // left/right-most members should be set to the sentinel value.
            ASSERT_NULL(tree.root_, "");
            ASSERT_EQ(tree.sentinel(), tree.left_most_,  "");
            ASSERT_EQ(tree.sentinel(), tree.right_most_, "");
            EXPECT_EQ(0u, tree.size(), "");
        } else {
            // If the tree is non-empty, then the root, left-most and
            // right-most pointers should all be valid.  The LR-child of the
            // LR-most element should be the sentinel value.
            ASSERT_NONNULL(tree.root_, "");
            ASSERT_FALSE(is_sentinel_ptr(tree.root_), "");

            ASSERT_NONNULL(tree.left_most_, "");
            ASSERT_FALSE(is_sentinel_ptr(tree.left_most_), "");
            ASSERT_EQ(tree.sentinel(),
                       NodeTraits::node_state(*tree.left_most_).left_,
                       "");

            ASSERT_NONNULL(tree.right_most_, "");
            ASSERT_FALSE(is_sentinel_ptr(tree.right_most_), "");
            ASSERT_EQ(tree.sentinel(),
                       NodeTraits::node_state(*tree.right_most_).right_,
                       "");

            EXPECT_LT(0u, tree.size(), "");
        }

        // Compute the size and depth of the tree, verifying the parent
        // back-links as we go.  If this checker is being used as part of the
        // balance test, we will also verify the rank rule as we go as well as
        // the balance bounds at the end.
        uint64_t cur_depth = 0;
        uint64_t depth     = 0;
        size_t   size      = 0;

        RawPtrType node = tree.root_;

        // Start by going left until we have determined the depth of the
        // left most node of the tree.
        while (valid_sentinel_ptr(node)) {
            ASSERT_TRUE(VerifyParentBackLinks(tree, node), "");

            auto& ns = NodeTraits::node_state(*node);
            ++cur_depth;

            if (!valid_sentinel_ptr(ns.left_))
                break;

            node = ns.left_;
        }

        // Now walk through the tree in ascending key order.  The basic
        // idea is...
        //
        // 1) "Visit" our current node, recording the new max depth if we
        //    have discovered one and checking the rank rule.
        // 2) If there is a right hand child, advance to the left-most node
        //    of the right hand sub-tree.
        // 3) If there is no right hand child, climb the tree until we
        //    traverse a left hand link.
        // 4) If we don't traverse a left hand link before hitting the root
        //    of the tree, then we are done.
        while (valid_sentinel_ptr(node)) {
            // #1: Visit
            if (depth < cur_depth)
                depth = cur_depth;
            ++size;

            ASSERT_TRUE(VerifyParentBackLinks(tree, node), "");

            // Verify the rank rule using the tree's observer's implementation.
            // If this sanity check is being run as part of the balance test, it
            // will use the implementation above.  For all other tests, only the
            // rank parity is stored in the node, so we cannot rigorously verify
            // the rule.  The default VerifyRankRule implementation will be used
            // instead (which just returns true).
            ASSERT_TRUE(Observer::VerifyRankRule(tree, node), "");

            // #2: Can we go right?
            const auto& ns = NodeTraits::node_state(*node);
            if (valid_sentinel_ptr(ns.right_)) {
                cur_depth++;
                node = ns.right_;
                ASSERT_TRUE(VerifyParentBackLinks(tree, node), "");

                // Now go as far left as we can.
                while (true) {
                    auto& ns = NodeTraits::node_state(*node);

                    if (!valid_sentinel_ptr(ns.left_))
                        break;

                    ++cur_depth;
                    node = ns.left_;
                    ASSERT_TRUE(VerifyParentBackLinks(tree, node), "");
                }

                // Loop and visit the next node.
                continue;
            }

            // #3: Climb until we traverse a left-hand link.
            RawPtrType parent = ns.parent_;
            bool keep_going;
            while ((keep_going = valid_sentinel_ptr(parent))) {
                auto& parent_ns = NodeTraits::node_state(*parent);
                bool is_left    = (parent_ns.left_  == node);
                bool is_right   = (parent_ns.right_ == node);

                ASSERT_TRUE(is_left != is_right, "");
                ASSERT_TRUE(is_left || is_right, "");

                node = parent;
                --cur_depth;

                if (is_left)
                    break;

                parent = parent_ns.parent_;
            }

            // #4: If we ran out of valid parents to climb past, we are done.
            if (!keep_going)
                break;
        }

        // We should have visited all of the nodes by now.
        ASSERT_EQ(tree.size(), size, "");

        // If this is being called from the balance tests, check the balance
        // bounds, and computational complexity bounds.
        ASSERT_TRUE(Observer::VerifyBalance(tree, depth), "");

        END_TEST;
    }
};

}  // namespace intrusive_trees
}  // namespace tests
}  // namespace fbl
