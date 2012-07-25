import Image
import gameduino.prep as gdprep

out=open("sprites.h", "w")
ir = gdprep.ImageRAM(out)
im= Image.open("sprites/jaromil.png")
jw = gdprep.palettize(im, 4)
ir.addsprites("jaromil", (16, 32), jw, gdprep.PALETTE4A, center = (8,16))
gdprep.dump(out, "sprites", ir.used())
# fucks up order of colors somehow :(
#gdprep.dump(out, "jaromil_palette", gdprep.getpal(im))
