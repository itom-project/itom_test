#!/usr/bin/env python3

'''
This little script will copy binary runtime dependencies such as frameworks and dylibs into the application bundle.
'''
from __future__ import print_function
import glob
import sys
import os
import os.path
import sys
import subprocess
import shutil
import stat

recursive = False


# Check for OS X
if sys.platform != 'darwin':
	raise NameError("This script does only make sence on OS X systems\nQuitting")

# CLI colors
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def rchmod(path,perm):
	# Recursive permission change
	for root, dirs, files in os.walk(path):  
		for momo in dirs:
			os.chmod( os.path.join(root, momo),perm)
		for momo in files:
			os.chmod( os.path.join(root, momo),perm)

def getDependList(path):
	outp = subprocess.check_output(["otool", "-L", path]).decode("utf-8")
	outp = outp.replace("\t","")
	sp = int(outp.find("\n")+1)
	if sp >= 0:
		outp = outp[sp:]
	outp = outp.split('\n')
	return outp

# Copy dependencies to bundle
def makeBundle(path):
	if not os.path.isfile(path):
		raise NameError('File "'+path+'" doesn\'t exist')

	# Getting dependencies (otool) and parsing files
	app_path = path
	if path.find(".app") >= 0:
		app_path = path[:path.rfind(".app")+4]
	elif path.find(".framework") >= 0:
		app_path = path[:path.rfind(".framework")+10]
	elif path.find(".dylib") >= 0:
		app_path = path[:path.rfind(".dylib")+6]

	app_name = app_path
	if app_path.rfind('/')+1 < len(app_path):
		app_name = app_path[app_path.rfind('/')+1:]
	fwk_path = app_path+'/Contents/Frameworks'
	fwk_appp = app_name+'/Contents/Frameworks'
	print(bcolors.OKGREEN+'Processing '+path[path.rfind('/')+1:]+' in '+app_name+bcolors.ENDC)
	outp = getDependList( path)

	if len(outp) < 1:
		print(bcolors.WARNING+"No dependencies found"+bcolors.ENDC)
		return 0

	# # Remove framework dir
	# if os.path.isdir(fwk_path):
	# 	shutil.rmtree(fwk_path)
	# 	print(bcolors.OKBLUE+'Framework directory removed'+bcolors.ENDC)
	
	# Make framework dir
	if not os.path.isdir(fwk_path):
		os.mkdir(fwk_path)
		print(bcolors.OKBLUE+'Framework directory made'+bcolors.ENDC)

	# Loop files
	offset = "                    "
	idx = 0 # Index/ Number of dependencies
	for line in outp:
		sp = int(line.find(" ("))
		if sp >= 0:
			line = line[:sp]

		if line:
			if line.find("/Library") == 0:
				# Ignore system wide libs
				continue

			if line.find('.framework') >= 0:
				# Framework
				name = line[line.rfind('/')+1:]
				print(bcolors.OKBLUE+"Framework:        "+bcolors.ENDC, bcolors.BOLD, name+'.framework', bcolors.ENDC)
				fwk_base = line[:line.rfind('/')]
				ver_path = line[line.rfind('.framework/')+10:line.rfind('/')]
				dst_path = fwk_path + "/" + name + '.framework' + ver_path
				dst_name = fwk_path+'/'+line[line.rfind('/',0,line.rfind('.framework/'))+1:]	

				if line.find("@executable_path") == 0:
					print(offset+bcolors.WARNING+"Already adapted"+bcolors.ENDC)
					continue

				if not os.path.isfile(line):
					# Dependency exists?
					print(offset+bcolors.FAIL+"Dependency doesn't exist. Skipping file."+bcolors.ENDC)
					continue

				if not os.path.isfile(dst_name):
					# Copy file
					os.makedirs(dst_name[:dst_name.rfind('/')])
					ret = subprocess.call(["cp", "-r", fwk_base+'/', dst_path])
					if ret == 0:
						print(offset+"Copied file")
					else:
						print(offset+bcolors.FAIL+"Failed to copy file"+bcolors.ENDC)
				else:
					print(offset+"File already exists")

				# Change file permissions
				rchmod( dst_path, stat.S_IRWXU | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)# | stat.S_IRWXG | stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP | stat.S_IRWXO | stat.S_IROTH | stat.S_IWOTH | stat.S_IXOTH)
				print(offset+"File permissions set")

				# Setting the identification names for the frameworks
				ret = subprocess.call(["install_name_tool", "-id", "@executable_path/../Frameworks/"+line[line.rfind('/',0,line.rfind('.framework/'))+1:], dst_name])
				if ret == 0:
					print(offset+"Identification name set")
				else:
					print(offset+bcolors.FAIL+"Failed to set identification name"+bcolors.ENDC)

				# Making the application aware of the library locations
				ret = subprocess.call(["install_name_tool", "-change", line, "@executable_path/../Frameworks/"+line[line.rfind('/',0,line.rfind('.framework/'))+1:], path])
				if ret == 0:
					print(offset+"Setting new library position")
				else:
					print(offset+bcolors.FAIL+"Failed to set new library position"+bcolors.ENDC)

				idx += 1

				# Check dependencies of dependencie
				if recursive:
					makeBundle(line)
			elif line.find('.dylib') >= 0:
				# Dynamic Library
				name = line[line.rfind('/')+1:]
				print(bcolors.OKBLUE+"Dynamic Library:  "+bcolors.ENDC, bcolors.BOLD, name, bcolors.ENDC)	

				if line.find("@executable_path") == 0:
					print(offset+bcolors.WARNING+"Already adapted"+bcolors.ENDC)
					continue

				if not os.path.isfile(line):
					# Dependency exists?
					print(offset+bcolors.FAIL+"Dependency doesn't exist. Skipping file."+bcolors.ENDC)
					continue

				if not os.path.isfile(fwk_path+"/"+name):
					# Copy file
					ret = subprocess.call(["cp", "-r", line, fwk_path])
					if ret == 0:
						print(offset+"Copied file")
					else:
						print(offset+bcolors.FAIL+"Failed to copy file"+bcolors.ENDC)
				else:
					print(offset+"File already exists")

				# Change file permissions
				os.chmod( fwk_path+"/"+name, stat.S_IRWXU | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)# | stat.S_IRWXG | stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP | stat.S_IRWXO | stat.S_IROTH | stat.S_IWOTH | stat.S_IXOTH)
				print(offset+"File permissions set")

				# Setting the identification names for the frameworks
				ret = subprocess.call(["install_name_tool", "-id", "@executable_path/../Frameworks/"+name, fwk_path+"/"+name])
				if ret == 0:
					print(offset+"Identification name set")
				else:
					print(offset+bcolors.FAIL+"Failed to set identification name"+bcolors.ENDC)

				# Making the application aware of the library locations
				ret = subprocess.call(["install_name_tool", "-change", line, "@executable_path/../Frameworks/"+name, path])
				if ret == 0:
					print(offset+"Setting new library position")
				else:
					print(offset+bcolors.FAIL+"Failed to set new library position"+bcolors.ENDC)

				idx += 1

				# Check dependencies of dependencie
				if recursive:
					makeBundle(line)
			else:
				# Other type
				print(bcolors.OKBLUE+"Unrecognised Type: "+bcolors.ENDC, bcolors.BOLD, line[line.rfind('/')+1:], bcolors.ENDC)
				print(offset+bcolors.WARNING+"will be ignored"+bcolors.ENDC)
				raise NameError('Quitting')

	return idx

# Main
if __name__ == '__main__':
	print(bcolors.HEADER+'OS X Bundle Maker'+bcolors.ENDC)
	print(bcolors.HEADER+'================='+bcolors.ENDC)
	print(bcolors.HEADER+''+bcolors.ENDC)
	print(bcolors.HEADER+'Fill a bundle (application or framework) with all dependencies. Frameworks or dynamic libraries.'+bcolors.ENDC)
	print(bcolors.HEADER+''+bcolors.ENDC)
	print(bcolors.HEADER+'Author: 	Jan Beneke <dev@janbeneke.de>'+bcolors.ENDC)
	print(bcolors.HEADER+'Version:	1.0'+bcolors.ENDC)
	print(bcolors.HEADER+'License:	BSD'+bcolors.ENDC)

	nargs = len(sys.argv)
	path = ''

	if nargs > 1:
		path = str(sys.argv[1])

	if not path:
		print("Looking for Application bundles")
		for file in glob.glob("*.app"):
			name = file.rsplit('.', 1)[0]
			path = os.getcwd()+"/"+file+"/Contents/MacOS/"+name
			makeBundle(path)
	elif os.path.isfile(path):
		makeBundle(path)
	else:
		raise NameError(bcolors.FAIL+'Invalid arguments passed'+bcolors.ENDC)
	print(bcolors.OKGREEN+'Finished'+bcolors.ENDC)

	print(bcolors.FAIL+'TODO: RECURSION!!!'+bcolors.ENDC)

