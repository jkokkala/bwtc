from utils import *
import os
import sys
import matplotlib.pyplot as plt
import numpy as np
from config import path
os.chdir(path)
#os.chdir("/home/jkokkala/bwtc/")
encoders = {"Huffman":"H","Wavelet": "W", "MTF-RLE0":"0","MTF":"F", "MTF-RLE":"f"}
#encoders = {"Huffman":"H","MTF":"F", "MTFRL":"f"}
preprocessors = ["pp"]
inputs= {"XML50M":"inputs/dblp.xml.50MB","DNA50M":"inputs/dna.50MB","English50M":"inputs/english.50MB","Wiki50M":"inputs/enwik8.50MB","Kernel50M":"inputs/sources.50MB"}
comp_algs = ["precompress","doTransform","encodeData"]
decomp_algs=["uncompress","doTransform","decodeBlock"]
#inputs={"Wiki":"inputs/wiki"}
temp_output="temp/output"
data_entries=[]
for inp in inputs.keys():
    for preprocessor in preprocessors:
        for encoder in encoders.keys():
            db=load_db()
            entry = find(db,{"input":inp,"preprocessor":preprocessor,"encoder":encoder})
            if(entry != None):
                data_entries.append(entry)
                continue
            (compression_time,output) = time_process("bin/compress --prepr "+preprocessor+" --enc "+encoders[encoder]+" "+inputs[inp]+" "+temp_output)
            compression_totaltime=get_times(output,["::compress"])[0]
            compression_subtimes = get_times(output,comp_algs)
            original_size = file_size(inputs[inp])
            compressed_size = file_size(temp_output)
            compression_ratio = compressed_size*1.0/original_size
            (decompression_time,output) = time_process("bin/uncompress "+temp_output)
            decompression_totaltime=get_times(output,["::decompress"])[0]
            decompression_subtimes = get_times(output,decomp_algs)
            print preprocessor+"/"+encoder+"/"+inp+":"
            
            print "-- Compression time: {0:.3f}".format(compression_time)
            comp_alg_results=[]
            decomp_alg_results=[]
            for i in range(len(comp_algs)):
                comp_alg_results.append(compression_time*compression_subtimes[i]/compression_totaltime)
                print ("  -- "+comp_algs[i]+":  {0:.3f}").format(compression_time*compression_subtimes[i]/compression_totaltime)
            print "-- Decompression time: "+str(decompression_time)
            for i in range(len(decomp_algs)):
                decomp_alg_results.append(decompression_time*decompression_subtimes[i]/decompression_totaltime)
                print ("  -- "+decomp_algs[i]+": {0:.3f}").format(decompression_time*decompression_subtimes[i]/decompression_totaltime)
            print "-- Compression ratio: "+str(compression_ratio)
            item = {"input":inp,"preprocessor":preprocessor,"encoder":encoder,"compression_times":comp_alg_results,"decompression_times": decomp_alg_results,"compression_ratio":compression_ratio}
            db.append(item)
            save_db(db)
            data_entries.append(item)

save_backup()


fig=plt.figure()
ax=fig.add_subplot(111)

legend=[]
rects=[]

N=len(inputs)
width=0.35
ind = np.arange(N)*(0.5+len(encoders)*width)


it=0
mx=0
for enc in encoders:

    legend.append(enc)
    entries = [e for e in data_entries if e["encoder"]==enc]
    ratios=[]
    labels=[]
    for a in entries:
        ratios.append(a["decompression_times"][2])
        labels.append(a["input"])
    if(max(ratios)>mx):
        mx=max(ratios)
    rects.append(ax.bar(ind+it*width,ratios,width,color=getcolor()))
    it+=1




ax.set_ylabel('Decompression time')
ax.set_title('Decompression times')
ax.set_xticks(ind+len(rects)*width/2)
ax.set_xticklabels(tuple(labels))
#plt.yticks(np.arange(0,mx+0.2,0.1))
ax.legend(tuple([r[0] for r in rects]),tuple(legend))
plt.show()

    



