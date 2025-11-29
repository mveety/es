#!/usr/bin/env es

. macroreplace.es
macroreplace es.1.src | groff -eTpdf -man > es.1.pdf

