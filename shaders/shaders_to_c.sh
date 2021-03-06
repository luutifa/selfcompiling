#!/bin/sh

TERM=xterm # Needed to avoid Mono bugs
SOURCEFILES=`find * -name '*.frag' -o -name '*.vert'`
OUTPUTFILE="../shaders.h.out"
if [ -z "$SHADER_MINIFIER" ]; then
    SHADER_MINIFIER=~/misc/shader_minifier.exe
fi

# Minify with Crtl-Alt-Test's tool when needed
if [ ! "$1" = "debug" ]; then
    for f in $SOURCEFILES; do
        mono $SHADER_MINIFIER --format none -o "$f.min" "$f"
    done
fi

# Convert to C source
for f in $SOURCEFILES; do
    # Generate C identifiers/symbols for filenames
    F=`echo $f | tr ./ _ | tr '[:lower:]' '[:upper:]'`

    # Write C source code, only use minified when not debugging
    printf "char *$F=\"" > "$f.h.out"
    if [ "$1" = "debug" ]; then
        printf "shaders/$f" >> "$f.h.out" # only write filename
    else
        cat "$f.min" | sed 's/$/\\/' >> "$f.h.out" # useless use of cat?
        echo >> "$f.h.out"
    fi
    echo "\";" >> "$f.h.out"
done

# Concatenate all shaders to a file
echo > $OUTPUTFILE
for f in *.h.out; do
    cat $f >> $OUTPUTFILE
done
echo >> $OUTPUTFILE
