#!/bin/bash

RELVALDIR=$PWD

HTMLDIR=$RELVALDIR
PRINT=false
COMPARE=1

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
    TAG1=$(basename $REL1)
    TAG2=$(basename $REL2)
elif [ $# -eq 4 ]; then
    REL1=$1
    REL2=$3
    FILE1=$2
    FILE2=$4
    if [ $REL1 = $REL2 ]; then
        TAG1=$(basename $REL1)_$(basename $FILE1 | sed 's|^\([^.]*\)[.].*$|\1|')
        TAG2=$(basename $REL2)_$(basename $FILE2 | sed 's|^\([^.]*\)[.].*$|\1|')
    else
        TAG1=$(basename $REL1)
        TAG2=$(basename $REL2)
    fi
else
    echo "Usage: relval.sh DIR1 [FILE1] DIR2 [FILE2]"
    exit 1
fi

echo "Checking directories.."

cd $REL1
eval `scramv1 runtime -sh` || exit 1
root -l -b -q $RELVALDIR'/testLibs.cc+("'$FILE1'")' || (echo "failed"; rm -f entries.txt; exit 1)

cd $REL2
eval `scramv1 runtime -sh` || exit 1
root -l -b -q $RELVALDIR'/testLibs.cc+("'$FILE2'")' || (echo "failed"; rm -f entries.txt; exit 1)

rm -f entries.txt

echo " Done."
echo "Dumping event contents.."

cd $REL1
eval `scramv1 runtime -sh` || exit 1
cd $RELVALDIR
root -l -b -q $RELVALDIR'/dumpEvents.cc+("'$TAG1'", "'$FILE1'", '$PRINT')' || (echo "failed"; rm histo_*.root; exit 1)

cd $REL2
eval `scramv1 runtime -sh` || exit 1
cd $RELVALDIR
root -l -b -q $RELVALDIR'/dumpEvents.cc+("'$TAG2'", "'$FILE2'", '$PRINT')' || (echo "failed"; rm histo_*.root; exit 1)

echo " Done."

if [ $COMPARE -eq 0 ]; then
    exit 0
fi

echo "Comparing distributions.."

root -l -b -q $RELVALDIR'/compare.cc+("'$TAG1'", "'$TAG2'", "'$HTMLDIR'")' || (echo "failed"; exit 1)
mv histo_${TAG1}.root $HTMLDIR
mv histo_${TAG2}.root $HTMLDIR

echo " Done."
