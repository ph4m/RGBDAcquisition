#!/bin/bash

#uses cbp2make , get it here - > http://sourceforge.net/projects/cbp2make/
cbp2make -in CodeBlocks.workspace -unix -out makefile

cd tools
cbp2make -in Tools.workspace -unix -out makefile
cd ..


if [ -d synergiesAdapter ]
then
 cd synergiesAdapter
 echo "Also updating synergies Adapter ( since we found it )" 
 cbp2make -in Adapter.cbp -unix -out makefile
 cd ..
fi


exit 0
