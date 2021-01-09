#!/usr/bin/env python3

import math
import re

print("Generating MaterialDesign.js")

# open css file from submodule
css = open("materialdesignicons/css/materialdesignicons.css")
print("Got css file!")

# lines to be written to js file
lines = ["var icons = {"]


# function to get the surrogate pair for js
def get_surrogate_pair(astral_code_point: int) -> tuple:
    high_surrogate = math.floor((astral_code_point - 0x10000) / 0x400) + 0xD800
    low_surrogate = (astral_code_point - 0x10000) % 0x400 + 0xDC00
    return str(hex(high_surrogate)).replace("0x", "").upper(), str(hex(low_surrogate)).replace("0x", "").upper()


for line in css:
    if "::before" in line:
        next_line = next(css)
        if "content:" in next_line:
            # found an icon
            icon_name = re.findall(r"mdi-[a-z-]+", line)[0].replace("-", "_")
            content_line = re.findall(r"\\[A-Z0-9]+", next_line)[0]
            hex_code = int(content_line.replace("\\F", "0xF"), 16)
            surrogate_pair = get_surrogate_pair(hex_code)
            js_pair = "\\u" + surrogate_pair[0] + "\\u" + surrogate_pair[1]
            # add icon line to js lines
            lines.append("  " + icon_name + ": \"" + js_pair + "\",")

lines.append("}")

# save to src js file
new_file = open("../src/qml/MaterialDesign.js", "w")
for line in lines:
    new_file.write(line + "\n")
