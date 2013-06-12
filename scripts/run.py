import utils
from utils import *
import os
import sys
import matplotlib.pyplot as plt
import numpy as np
from config import path

from matplotlib.backends.backend_pdf import PdfPages
#bwtc path
os.chdir(path)

#encoders = {"Huffman":"H","Wavelet": "W", "MTF":"F", "Gamma-RLE-MTF":"0", "Delta-RLE-MTF":"f",}
encoders = {"Arithm": "m", "Huffman":"H","Wavelet": "W", "MTF-Huffman":"F","RLEMTF-Huffman":"f","MTF-Arithm":"A","RLEMTF-Arithm":"a","new":"f"}
encoders = {"Arithm2": "m", "Arithm":"m","Huffman":"H"}
#encoders = {"Huffman":"H","MTF":"F", "MTFRL":"f"}
preprocessors = ["pp"]
inputs= {"XML50M":"inputs/dblp.xml.50MB","DNA50M":"inputs/dna.50MB","English50M":"inputs/english.50MB","Wiki50M":"inputs/enwik8.50MB","Kernel50M":"inputs/sources.50MB","Einstein.en.50M":"inputs/einstein.en.50MB","Kernel50M":"inputs/kernel.50MB"}
#inputs= {"English50M":"inputs/english.50MB"}
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



def compressionratio(a):
    return a["compression_ratio"]

def compressiontotaltime(a):
    return sum(a["compression_times"])

def decompressiontotaltime(a):
    return sum(a["decompression_times"])

def compressionentropytime(a):
    return a["compression_times"][2]

def decompressionentropytime(a):
    return a["decompression_times"][2]



plotdata = [
    {   "title": "Compression ratios",
        "values": compressionratio,
        "ytitle": "Compression ratio",
    },
    {   "title": "Entropy encoding time",
        "values": compressionentropytime,
        "ytitle": "Encoding time",
    },
    {   "title": "Entropy decoding time",
        "values": decompressionentropytime,
        "ytitle": "Decoding time",
    },
    {   "title": "Total compression time",
        "values": compressiontotaltime,
        "ytitle": "Compression time",
    },
    {   "title": "Total decompression time",
        "values": decompressiontotaltime,
        "ytitle": "Decompression time",
    },
]

pdf = PdfPages('scripts/stats.pdf')
for plottype in plotdata:
    utils.coloriter=0
    #    fig=plt.figure(figsize=(15,6))
    fig=plt.figure(figsize=(15,6))
    ax=fig.add_subplot(111)

    legend=[]
    rects=[]

    N=len(inputs)
    width=0.35
    ind = np.arange(N)*(0.5+len(encoders)*width)


    it=0
    mx=0
    for enc_ in sorted(encoders.iteritems()):
        enc=enc_[0]

        legend.append(enc)
        entries = [e for e in data_entries if e["encoder"]==enc]
        ratios=[]
        labels=[]
        for a in entries:
            ratios.append(plottype["values"](a))
            labels.append(a["input"])
        if(max(ratios)>mx):
            mx=max(ratios)
        rects.append(ax.bar(ind+it*width,ratios,width,color=getcolor()))
        it+=1




    ax.set_ylabel(plottype['ytitle'])
    ax.set_title(plottype['title'])
    ax.set_xticks(ind+len(rects)*width/2)
    ax.set_xticklabels(tuple(labels))
    #plt.yticks(np.arange(0,mx+0.2,0.1))
    legend=ax.legend(tuple([r[0] for r in rects]),tuple(legend),loc='center left',bbox_to_anchor = (1.0, 0.5))
    pdf.savefig(fig,bbox_extra_artists=(legend,), bbox_inches='tight')

pdf.close()

        



