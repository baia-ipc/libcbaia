#
# Bash library used by the Makefile test goal
#

#
# Run a command, check the exit code
# and compare up to two files with known results
#

function _run_diff { local RES=$1; EXP=$2 SOR=$3
  if [ "$EXP" != "" ]; then
    if [ "$SOR" -eq 1 ]; then
      echo "  Comparing (sorted) output ($RES) to expected results ($EXP)"
      RES1=$RES.s
      sort $RES > $RES1
      EXP1=$EXP.s
      sort $EXP > $EXP1
    else
      echo "  Comparing output ($RES) to expected results ($EXP)"
      RES1=$RES
      EXP1=$EXP
    fi
    diff $RES1 $EXP1
    CODE=$?
    if [ $CODE -ne 0 ]; then
      echo "  Comparison failed, differences found!"
      cp $RES $EXP.found
      echo "  Wrong file copied to $EXP.found"
      return 1
    fi
    if [ "$SOR" -eq 1 ]; then
      rm $EXP1 $RES1
    fi
  fi
  return 0
}

function run_cmp { local LBL=$1; # test label to display
                         CPX=$2; # command prefix (e.g. valgrind)
                         CMD=$3; # command to run and display
                         RED=$4; # redirect output to RES1? 0: no; 1: yes
                         RES1=$5; # first results file (opt; required if RED)
                         EXP1=$6; # first expected results file (if RES1)
                         SOR1=$7; # sort RES1/EXP1 before comparing? 0: no 1: y
                         RES2=$8; # second results file (optional)
                         EXP2=$9; # second expected results file (if RES2)
                         RES3=$10; # second results file (optional)
                         EXP3=${11};  # second expected results file (if RES3)
  echo -e "\nTest $LBL ..."
  echo "  Command: $CMD"
  #echo "${LBL} | ${CPX} | ${CMD} | ${RED} | ${RES1} | ${EXP1} | ${SOR1} | ${RES2} | ${EXP2} | ${RES3} | ${EXP3} |"
  
  FAILED=0
  
  if [ $RED -eq 0 ]
    then $CPX $CMD
    else $CPX $CMD > $RES1
  fi
  CODE=$?
  if [ $CODE -ne 0 ]; then
    echo "  Command failed, exit code: $CODE"
    FAILED=1
  fi

  if [ $FAILED -eq 0 ]; then
    if [ "$EXP1" != "" ]; then
      _run_diff "$RES1" "$EXP1" "$SOR1"
      FAILED=$?
    fi
  fi

  if [ $FAILED -eq 0 ]; then
    if [ "$EXP2" != "" ]; then
      _run_diff "$RES2" "$EXP2" 0
      FAILED=$?
    fi
    if [ "$EXP3" != "" ]; then
      _run_diff "$RES3" "$EXP3" 0
      FAILED=$?
    fi
  fi

  if [ $FAILED -eq 0 ]; then
    if [[ -t 1 ]]; then echo -n -e "\x1b[92m"; fi
    echo -e "\n[Test $LBL] SUCCESS"
    if [[ -t 1 ]]; then echo -n -e "\x1b[0m"; fi
  else
    if [[ -t 1 ]]; then echo -n -e "\x1b[91m"; fi
    echo -e "\n[Test $LBL] FAILED"
    if [[ -t 1 ]]; then echo -n -e "\x1b[0m"; fi
  fi

  return $FAILED
}
