#!/usr/bin/env bash
set -e
set -u

TMP=keys_tmp
OUT=profile_keys.c

if [[ ! -e moby_dick.txt ]]; then
  echo "Downloading moby dick txt ~1.2 MiB"
  curl http://www.gutenberg.org/files/2701/old/moby10b.txt -o moby_dick.txt
fi;

# generate 4 different orderings of the same keys
# the first is for insert / set
# the second for get
# the third for exists
# the fourth for delete
cat moby_dick.txt | tr -d "'"'"\n\r.,\?\!;:' | tr ' ' '\n' | sort | uniq | grep -v -E '^$' > ${TMP}

echo "" > ${OUT}

echo "const char *insert_keys[] = {" >> ${OUT}
cat ${TMP} | shuf | awk '{print "\"" $1 "\","}' >> ${OUT}
echo "};" >> ${OUT}
echo "const char **set_keys = insert_keys;" >> ${OUT}

echo "const char *get_keys[] = {" >> ${OUT}
cat ${TMP} | shuf | awk '{print "\"" $1 "\","}' >> ${OUT}
echo "};" >> ${OUT}

echo "const char *exists_keys[] = {" >> ${OUT}
cat ${TMP} | shuf | awk '{print "\"" $1 "\","}' >> ${OUT}
echo "};" >> ${OUT}

echo "const char *delete_keys[] = {" >> ${OUT}
cat ${TMP} | shuf | awk '{print "\"" $1 "\","}' >> ${OUT}
echo "};" >> ${OUT}

rm ${TMP}

