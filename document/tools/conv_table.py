# Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import re


def conv_table(table):
    def strip_list(l):
        n = len(l)
        if n == 0:
            return None
        for i in reversed(range(n)):
            if l[i] != '':
                break
        return l[:i+1]


    ncol = None
    rows = []
    row = []
    for n, l in enumerate(table):
        if n == 0:
            ncol = len(l.split('+')) - 2
            for c in range(ncol):
                row.append([])
        else:
            if len(l.split('+')) == ncol + 2:
                rows.append(row)
                row = []
                for c in range(ncol):
                    row.append([])
            if len(l.split('|')) == ncol + 2:
                ls = l.split('|')
                for c in range(ncol):
                    row[c].append(ls[c+1].strip())

    newrows = []
    for row in rows:
        if row != [[]]:
            newrows.append(row)
    rows = newrows

    newtable = []

    newtable.append('')
    newtable.append('')
    newtable.append('.. list-table::')
    if len(rows[0]) == 1:
        newtable.append('   :widths: 100')
    if len(rows[0]) == 2:
        newtable.append('   :widths: 30 70')
        
    if len(rows) > 1:
        newtable.append('   :class: longtable')
        #newtable.append('   :header-rows: 1')
    
    for row in rows:
        newtable.append('')
        h = '*'
        for col in row:
            c = strip_list(col)
            if c is None:
                continue
            if len(c) == 1:
                newtable.append(f'   {h} - {c[0]}')
            else:
                newtable.append(f'   {h} -')
                for l in c:
                    newtable.append(f'        {l}')
            h = ' '

    newtable.append('')
    newtable.append('')
    return newtable


lines = []
intable = False
with open(sys.argv[1]) as f:
    for l in f.readlines():
        l = l.rstrip()
        if intable:
            if l.startswith('+') or l.startswith('|'):
                table.append(l)
            else:

                intable = False
                lines += conv_table(table)
                table = None
        else:
            if l.startswith('+-'):
                intable = True
                table = []
                table.append(l)
            else:
                lines.append(l)

for l in lines:
    print(l)
