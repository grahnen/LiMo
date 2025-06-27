#!/bin/sh
rm -f dot_png/*
ls dots | sort -n | xargs -i dot -Tpng dots/{} -o dot_png/{}.png
