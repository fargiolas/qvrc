#!/usr/bin/python
#
# qvrc - a GLSL volume rendering engine
# well... engine... let's say prototype/proof of concept... hack?
#
# Copyright (C) 2017 Filippo Argiolas <filippo.argiolas@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA.



import numpy as np
import dicom

import matplotlib.pyplot as plt

from optparse import OptionParser

import os

def load_scan(path):
    files = os.listdir(path)
    print("path: {}".format(path))
    print("files: {}".format(len(files)))

    slices = [dicom.read_file(path + '/' + s) for s in files]
    # if (series != None):
    #     print series
    #     slices = filter(lambda x: x.SeriesNumber == series, slices)
    #    slices.sort(key = lambda x: int(x.InstanceNumber))
    #    slices.sort(key = lambda x: int(x.SliceLocation))

    print("slices: {}".format(len(slices)))

    return slices

parser = OptionParser()
parser.add_option("-d", "--dir", dest="dir", help="dicom directory")
parser.add_option("-o", "--output", dest="out", help="output raw file")
parser.add_option("-r", "--range", dest="range", nargs=2, help="selection range")
parser.add_option("--dump", action="store_true", dest="dump", help="dump first slice metadata")
parser.add_option("--saveimg", dest="imgname", help="saves slice preview")

#parser.add_option("-s", "--series", dest="series",help="dicom series")

(options, args) = parser.parse_args()

if options.dir is None:
    parser.error("No DICOM directory given")
if options.out is None:
    parser.error("No output file given")


slices = load_scan(options.dir)


# Sort slices in anatomical order (don't assume file numbering mean
# anything, nor InstanceNumber), SliceLocation seems the proper
# parameter for sorting but it's not always available... so Image
# Plane Module is the only trustable source for voxel sizes and
# positions

# ftp://dicom.nema.org/MEDICAL/dicom/2015b/output/chtml/part03/sect_C.7.6.2.html
# http://nipy.org/nibabel/dicom/dicom_orientation.html
# http://www.itk.org/pipermail/insight-users/2003-September/004762.html

# IOP cosines define the direction of the first volume row and the
# first volume column with respect to the patient
row_dir = np.array(slices[0].ImageOrientationPatient[0:3])
col_dir = np.array(slices[0].ImageOrientationPatient[3:6])
# if we take the normal vector we have the orientation of the slices
# with respect to the patient
n = np.cross(row_dir, col_dir)
# now we take the position of each slice along our normal direction
# so we can sort the slices in anatomical order and compute the slice thickness
normal_positions = []

for s in slices:
    ipp = np.array(s.ImagePositionPatient)
    pos = np.dot(n, ipp)
    normal_positions.append(pos)

    # print pos, s.SliceLocation

# print normal_positions

# sort both by normal_positions value
# python trickery to zip, unzip and untuplify
slices, normal_positions = (list(t) for t in (zip(*sorted(zip(slices, normal_positions), key=lambda t: t[1]))))

# print normal_positions

# print np.diff(np.array(normal_positions))

# exit()

if options.range:
    selection = slice(int(options.range[0]), int(options.range[1]), 1)
    print("selection: {}".format(selection))
    slices = slices[selection]

if options.dump:
    print("sample slice metadata")
    print(slices[0])

final_shape = list(slices[0].pixel_array.shape)
final_shape.append(len(slices))
final_shape = np.array(final_shape)

try:
    thickness = slices[0].SliceThicknes
except:
    thickness = np.diff(normal_positions[0:2])

voxel_size = np.array([slices[0].PixelSpacing[0], slices[0].PixelSpacing[1], slices[0].SliceThickness])
volume_scale = voxel_size * final_shape;
volume_scale /= volume_scale.max()

stack = np.array([s.pixel_array for s in slices])

print("save to: {}".format(options.out))
stack.tofile(options.out)

print("dataset shape: {}".format(final_shape))
print("voxel size: {}".format(voxel_size))
print("volume scale: {}".format(volume_scale))
print("bits (allocated, stored, high): {}, {}, {}".format(slices[0].BitsAllocated, slices[0].BitsStored, slices[0].HighBit))


if options.imgname is None:
    exit()

n_rows = 6
n_cols = 6

fig,ax = plt.subplots(n_rows,n_cols,figsize=[12,12])

for i in range(n_rows*n_cols):
    ind = i*len(slices)/n_rows/n_cols
    cur_ax = ax[int(i/n_rows),int(i % n_rows)]
    cur_ax.imshow(slices[ind].pixel_array,cmap="bone")
    cur_ax.set_aspect('equal')
    cur_ax.axis('off')

plt.margins(0)

plt.subplots_adjust(wspace=0.05, hspace=0.0)

plt.savefig(options.imgname, bbox_inches='tight', pad=0)
