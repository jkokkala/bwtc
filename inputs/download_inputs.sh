#enwik8
wget http://mattmahoney.net/dc/enwik8.zip
unzip enwik8.zip
head enwik8 -c 52428800 > enwik8.50MB
rm enwik8.zip enwik8

#dna
wget http://pizzachili.dcc.uchile.cl/texts/dna/dna.50MB.gz
gunzip dna.50MB.gz

#english
wget http://pizzachili.dcc.uchile.cl/texts/nlang/english.50MB.gz
gunzip english.50MB.gz

#xml
wget http://pizzachili.dcc.uchile.cl/texts/xml/dblp.xml.50MB.gz
gunzip dblp.xml.50MB.gz

#sources
wget http://pizzachili.dcc.uchile.cl/texts/code/sources.50MB.gz
gunzip sources.50MB.gz

#einstein
wget http://pizzachili.dcc.uchile.cl/repcorpus/real/einstein.en.txt.7z
7z e einstein.en.txt.7z
head einstein.en.txt -c 52428800 > einstein.en.50MB
rm einstein.en.t*

#kernel
wget http://pizzachili.dcc.uchile.cl/repcorpus/real/kernel.7z
7z e kernel.7z
head kernel -c 52428800 > kernel.50MB
rm kernel
rm kernel.7z
