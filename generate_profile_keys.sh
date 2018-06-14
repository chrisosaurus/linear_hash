#!/usr/bin/env bash
set -e
set -u

if [[ ! -e moby_dick.txt ]]; then
  echo "Downloading moby dick txt ~1.2 MiB"
  curl http://www.gutenberg.org/files/2701/old/moby10b.txt -o moby_dick.txt
fi;

echo "const char *keys[] = {" > profile_keys.c
cat moby_dick.txt | tr -d "'"'"\n\r.,\?\!;:' | tr ' ' '\n' | sort | uniq | grep -v -E '^$' | awk '{print "\"" $1 "\","}' >> profile_keys.c
echo "};" >> profile_keys.c

