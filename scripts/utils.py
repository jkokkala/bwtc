import os
import subprocess
import time
import re
import json
import shutil as sh
def file_size(filename):
    return os.stat(filename).st_size

def time_process(args):
    print args
    start = time.time()
    output = run_process(args)
    return (time.time()-start,output)

def run_process(args):
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out,err=proc.communicate()
    return err

def get_times(output,algs):
    times=[]
    for alg in algs:
        for line in output.split("\n"):
            match = re.search("^[^,]*"+alg+"[^,]*,\d+,(\d+)(.\d+)?$",line)
            if(match != None):
                times.append(int(match.group(1)))
    return times


def delete_file(filename):
    os.remove(filename)
colors=["r","y","g","b","m","c","k"]
coloriter=0
def getcolor():
    global coloriter
    coloriter+=1
    return colors[coloriter-1]

dbname='scripts/dbfile'
bkupfolder='scripts/bkdb/'

def load_db():
    if not os.path.isfile(dbname):
        return []    
    d=open(dbname)
    try:
        data=json.load(d)
    except:
        return []
    if 'entries' in data:
        return data["entries"]
    return []

def save_backup():
    sh.copy2(dbname,bkupfolder+time.strftime("%d%m%y-%H%M%S"))

def save_db(entries):
    st = {"entries":entries}
    db=open(dbname,'w')
    db.write(json.dumps(st,indent=4))
    db.close()

def find(db,fltr):
    for d in db:
        found=True
        for k in fltr.keys():
            if(d[k] != fltr[k]):
                 found= False
        if(found):
            return d
    return None

            

