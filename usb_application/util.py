
def HEX(vals):
    if vals is not None:
        if type(vals) is int:
            return "%.2x"%vals
        return ' '.join('%.2x'%x for x in vals)
    return ''
