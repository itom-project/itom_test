import matplotlib

if matplotlib.__version__ < '2.0.0':
    from .backend_itom_v1 import *
elif matplotlib.__version__ < '3.0.0':
    from .backend_itom_v2 import *
else:
    from .backend_itom_v3 import *