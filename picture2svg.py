from PIL import Image, ImageOps
import subprocess
import xml.etree.ElementTree as ET
import xml.dom.minidom
from sympy import root
import cv2
import csv
import numpy as np

# Prétraitement de l'image : niveaux de gris, contraste, seuillage binaire
import os

#use mkbitmap and potrace to convert image to svg usefull if I use C
def mkbitmap_convert(input_path, output_png, output_svg):
    bmp_input = os.path.splitext(input_path)[0] + "_tmp_in.bmp"
    Image.open(input_path).convert("L").save(bmp_input)
    bmp_output = os.path.splitext(output_png)[0] + "_tmp_out.bmp"
    cmd = ["mkbitmap", "-f", "2","-b", "0.8", "-s", "2", "-t", "0.48", bmp_input, "-o", bmp_output]
    subprocess.run(cmd, check=True)

    cmd = ["potrace", "-s", "-t", "5", "--group", bmp_output, "-o", output_svg]
    subprocess.run(cmd, check=True)    
    Image.open(bmp_output).save(output_png)
    os.remove(bmp_input)
    os.remove(bmp_output)

    print(f"Fichier PNG final créé : {output_png}")

# Exemple d’utilisation :
input_jpg = "C:\\Users\\ryanm\\Documents\\Rmvi\\animation\\video\\fourier\\Image\\ryan.jpg"
output_png= "C:\\Users\\ryanm\\Documents\\Rmvi\\animation\\video\\fourier\\Image\\ryan_mkbitmap_contours.png"
output_svg= "C:\\Users\\ryanm\\Documents\\Rmvi\\animation\\video\\fourier\\Image\\ryan_mkbitmap_contours.svg"

#mkbitmap_convert(input_jpg, output_png, output_svg)
