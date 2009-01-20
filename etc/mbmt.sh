#! /bin/sh

# Mbmt: Memory-based machine translation
#
# 
#
# mbmt.sh is a script that follows the default procedure to train
# a translation model and a WOPR language model, and to translate
# a test text.
#
# Usage: mbmt.sh training test
#
# training is a GIZA++ aligned A3.final file;
# test is a source-language text, one sentence per line.
#
# Produces a translation of test in file test.out.
# 
# Assumes Timbl and Wopr installed;
# see http://ilk.uvt.nl/timbl and http://ilk.uvt.nl/wopr

if [ $# -ne 2 ]; then

 echo Usage: mbmt.sh training test
 echo " "
 echo training is a GIZA++ aligned A3.final file;
 echo test is a source-language text, one sentence per line.
 echo " "

else

    # preparation 0: give heads up
    echo preparing data...

    # preparation 1: create training data for translation model
    mbmt-0.1/mbmt-create-training $1 > $1.111.inst

    # preparation 2: create training data for language model
    mbmt-0.1/mbmt-tar-from-A3 $1

    # preparation 3: WOPR creates language model
    #
    echo Training LM
    wopr -s mbmtt.wopr >> /dev/null 2>&1 
    echo Starting LM server
    wopr -s mbmts.wopr >> /dev/null 2>&1 &

    # preparation 4: test material is converted
    mbmt-0.1/mbmt-create-test $2 > $2.111.inst

    # training and testing
    echo training and testing
    Timbl -f $1.111.inst -I $1.111.inst.ibase -a1 +vdb+di +D -Beam=1 -t $2.111.inst -o $2.111.inst.out > /dev/null 2>&1

    # decoding
    mbmt-decode $2.111.inst.out > $2.out

    # cleanup
    rm -f $1.111.inst $2.111.inst $2.111.inst.out > /dev/null 2>&1

fi
