
def HEX(vals):
    if vals is not None:
        return ' '.join('%.2x'%x for x in vals)

