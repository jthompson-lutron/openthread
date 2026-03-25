#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import unittest

import config
import thread_cert


class test_router_reattach(thread_cert.TestCase):
    TOPOLOGY = {
        1: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        2: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        3: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        4: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        5: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        6: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        7: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        8: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        9: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        10: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        11: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        12: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        13: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        14: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        15: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        16: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        17: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        18: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        19: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        20: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        21: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        22: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        23: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        24: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        25: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        26: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        27: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        28: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        29: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        30: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        31: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
        32: {
            'mode': 'rdn',
            'router_administration_profile': 'TestMaxThresholdsLowJitter'
        },
    }

    def test(self):
        self.nodes[1].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[1].get_state(), 'leader')

        for i in range(2, 33):
            self.nodes[i].start()
            self.simulator.go(config.ROUTER_STARTUP_DELAY)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        self.nodes[2].reset()

        self.nodes[2].set_router_administration_profile("TestMaxThresholdsLowJitter")

        self.nodes[2].start()
        self.assertEqual(self.nodes[2].get_router_administration_profile(), "TestMaxThresholdsLowJitter")

        # Verify that the node restored as Router.
        self.simulator.go(1)
        self.assertEqual(self.nodes[2].get_state(), 'router')
        # Verify that the node does not downgrade after Router Selection Jitter.
        self.simulator.go(5)
        self.assertEqual(self.nodes[2].get_state(), 'router')


if __name__ == '__main__':
    unittest.main()
