from svgpathtools import svg2paths

# Charge le SVG
paths, attributes = svg2paths("fourier.svg")

# Prend le premier chemin
path = paths[0]

# Échantillonne le chemin (par ex. 100 points)
num_points = 100
points = [path.point(i / num_points) for i in range(num_points + 1)]

# Sauvegarde en CSV
with open("points.csv", "w") as f:
    for p in points:
        f.write(f"{p.real},{p.imag}\n")

print("✅ points.csv généré avec succès !")
