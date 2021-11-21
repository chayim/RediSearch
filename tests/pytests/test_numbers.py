# -*- coding: utf-8 -*-

import unittest
from includes import *
from common import *
from time import sleep
from RLTest import Env
import math

def testEmptyNumericLeakIncrease(env):
    # test numeric field which updates with increasing value
    env.skipOnCluster()

    conn = getConnectionByEnv(env)
    conn.execute_command('FT.CREATE', 'idx', 'SCHEMA', 'n', 'NUMERIC')
    pl = conn.pipeline()

    repeat = 10
    docs = 100

    for i in range(repeat):
        for j in range(docs):
            x = j + i * docs
            pl.execute_command('HSET', 'doc{}'.format(j), 'n', format(x))
        results = pl.execute()
        #for res in results:
        #    print(res)
