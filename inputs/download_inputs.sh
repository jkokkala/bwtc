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
