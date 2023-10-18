#try to figure out where on earth python wants to stash the modules
#untested on anything but debian w/python 2.7, it may vary 
#depending on platform and python version
import os
import sys

version_str = str(sys.version_info.major) + "." + str(sys.version_info.minor)
pyinstall_path = os.path.join("lib", "python" + version_str, "site-packages")
print(pyinstall_path)
#print( os.path.join("lib", "python" + sys.version[:3],"site-packages") )
