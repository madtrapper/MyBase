import os
import sys
from os import walk
import shutil
from os import listdir
from os.path import isfile, join

sync_file_ext = [".cc", ".h", ".c"]
total_files = 0
missing_file = 0

def copyFile(src, dest):
    try:
        shutil.copy(src, dest)
    # eg. src and dest are the same file
    except shutil.Error as e:
        print('Error: %s' % e)
    # eg. source or destination doesn't exist
    except IOError as e:
        print('Error: %s' % e.strerror)

if __name__ == '__main__':
    dstDir = sys.argv[1]
    srcDir = sys.argv[2]
    action = sys.argv[3]
    print "Mybase dir: " + dstDir + "\nChrome dir: "+srcDir
    
    for dirpath, _, files in os.walk(dstDir):
        #path = root.split('/')
        #print (len(path) - 1) *'---' , os.path.basename(root)       
        for f in files:
            p = os.path.abspath(os.path.join(dirpath, f))
            mainName, ext = os.path.splitext(f)
            #print len(path)*'---', files
            if ext in sync_file_ext:
                total_files = total_files + 1
                p1, p2 = p.split(dstDir)
                srcFileName = srcDir + p2
                if os.path.exists(srcFileName):
                    if action == "replace":
                        copyFile(srcFileName, p)
                else:
                    missing_file = missing_file + 1
                    print "Src not found:" + srcFileName
                
    
    print "total_files: %d" % total_files
    print "Missing files : %d" % missing_file
