# -*- coding: utf-8 -*-
"""WARPNet package setup."""

import sys

from distutils.core import setup

# WARPNet requires at least 2.7
if not ((2, 7) <= sys.version_info):
    print("WARPNet requires at least Python 2.7.  Exiting")
    sys.exit(1)

# TBD:  Other setup

with open('README') as readme_file:
    long_description = readme_file.read()

setup( 
    name = 'warpnet',
    description = 'Toolchain for Python WARPNet',
    version = '2.0.0',
    author = 'Mango Communications',
    author_email = 'info@mangocomm.com',
    
    license = 'LICENSE',
    long_description = long_description,
    
    package_dir = {'warpnet':''},
    packages = ['warpnet'],
    scripts = ['']    
    
)
