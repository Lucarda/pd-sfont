#!/bin/sh

./localdeps.win.sh ./fluid~.dll

for filename in *.w32; do
	./localdeps.win.sh "$filename"
done