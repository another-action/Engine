#!/usr/bin/env python

import subprocess as p
                                    # Legacy Examples
cases = [ "run_stress.py",          # 67
	  "run_xvaexplain.py",      # 70
	  "run_sensi.py",           # 68
          "run_sacva.py",           # 68
          "run_bacva.py"            # 68
         ]

for c in cases:
    cmd = "python3 " + c;
    print ("Calling:", cmd)
    p.call(cmd, shell=True)
