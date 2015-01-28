#!/bin/bash

RELVALDIR=$PWD

HTMLDIR=$RELVALDIR
PRINT=false
COMPARE=1
DOKTEST=0
USERTAG=0
SKIP=""

while [ -n "$1" ]; do
    case $1 in
        -d)
            HTMLDIR=$2
            shift; shift
            ;;
        -v)
            PRINT=true
            shift
            ;;
        -n)
            COMPARE=0
            shift
            ;;
        -k)
            DOKTEST=1
            KTESTCUT=$2
            shift; shift
            ;;
	-s)
	    SKIP=$2
	    shift; shift
	    ;;
        -t)
            USERTAG=1
            TAG1=$2
            TAG2=$3
            shift; shift; shift
            ;;
        *)
            break
    esac
done

rm -rf $HTMLDIR/img > /dev/null 2>&1

SUBDIR=src/SusyAnalysis/SusyNtuplizer/macro

if [ $# -eq 2 ]; then
    REL1=$1
    REL2=$2
    FILE1=$REL1/$SUBDIR/susyEvents.root
    FILE2=$REL2/$SUBDIR/susyEvents.root
    if [ $USERTAG -eq 0 ]; then
        TAG1=$(basename $REL1)
        TAG2=$(basename $REL2)
    fi
elif [ $# -eq 4 ]; then
    REL1=$1
    REL2=$3
    FILE1=$2
    FILE2=$4
    if [ $USERTAG -eq 0 ]; then
        if [ $REL1 = $REL2 ]; then
            TAG1=$(basename $REL1)_$(basename $FILE1 | sed 's|^\([^.]*\)[.].*$|\1|')
            TAG2=$(basename $REL2)_$(basename $FILE2 | sed 's|^\([^.]*\)[.].*$|\1|')
        else
            TAG1=$(basename $REL1)
            TAG2=$(basename $REL2)
        fi
    fi
else
    echo "Usage: relval.sh DIR1 [FILE1] DIR2 [FILE2]"
    exit 1
fi

echo "Dumping event contents.."

cd $REL1
eval `scramv1 runtime -sh` || exit 1
cd $RELVALDIR
root -n -l -b -q "$RELVALDIR/dumpEvents.cc+(\"$TAG1\", \"$FILE1\", \"$SKIP\", \"\", $PRINT)" || (echo "failed"; rm histo_*.root; exit 1)

cd $REL2
eval `scramv1 runtime -sh` || exit 1
cd $RELVALDIR
if [ $DOKTEST -eq 1 ]; then
    root -n -l -b -q "$RELVALDIR/dumpEvents.cc+(\"$TAG2\", \"$FILE2\", \"$SKIP\", \"histo_$TAG1.root\", $PRINT)" || (echo "failed"; rm histo_*.root; exit 1)
else
    root -n -l -b -q "$RELVALDIR/dumpEvents.cc+(\"$TAG2\", \"$FILE2\", \"$SKIP\", \"\", $PRINT)" || (echo "failed"; rm histo_*.root; exit 1)
fi

echo " Done."

if [ $COMPARE -eq 0 ]; then
    exit 0
fi

echo "Comparing distributions.."

if [ $DOKTEST -eq 1 ]; then
    root -n -l -b -q "$RELVALDIR/compare.cc+(\"$TAG1\", \"$TAG2\", \"$HTMLDIR\", $KTESTCUT, true)" || (echo "failed"; exit 1)
else
    root -n -l -b -q "$RELVALDIR/compare.cc+(\"$TAG1\", \"$TAG2\", \"$HTMLDIR\")" || (echo "failed"; exit 1)
fi
mv histo_${TAG1}.root $HTMLDIR
mv histo_${TAG2}.root $HTMLDIR

echo " Done."
