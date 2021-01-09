#!/bin/sh

./localdeps.win.sh ./fluid~.dll

for filename in *.w64; do
	./localdeps.win.sh "$filename"
done