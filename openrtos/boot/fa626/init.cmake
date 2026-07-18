# Convert a hexadecimal number to decimal
# Arguments:
#   from_hex: The hexadecimal number to be converted
#   HEX: The input hexadecimal number (passed by reference)
#   DEC: The output decimal number (passed by reference)
function(from_hex HEX DEC)
    # Convert the input HEX to uppercase
    string(TOUPPER "${HEX}" HEX)
    # Remove "0x" from HEX
    string(REGEX REPLACE "^0X" "" HEX ${HEX})
    # Initialize _res variable to 0
    set(_res 0)
    # Get the length of HEX
    string(LENGTH "${HEX}" _strlen)

    # Loop through each character in HEX
    while (_strlen GREATER 0)
        # Multiply the result by 16
        math(EXPR _res "${_res} * 16")
        # Get the first character (nibble) of HEX
        string(SUBSTRING "${HEX}" 0 1 NIBBLE)
        # Remove the first character from HEX
        string(SUBSTRING "${HEX}" 1 -1 HEX)

        # Convert the nibble to its decimal value and add it to the result
        if (NIBBLE STREQUAL "A")
            math(EXPR _res "${_res} + 10")
        elseif (NIBBLE STREQUAL "B")
            math(EXPR _res "${_res} + 11")
        elseif (NIBBLE STREQUAL "C")
            math(EXPR _res "${_res} + 12")
        elseif (NIBBLE STREQUAL "D")
            math(EXPR _res "${_res} + 13")
        elseif (NIBBLE STREQUAL "E")
            math(EXPR _res "${_res} + 14")
        elseif (NIBBLE STREQUAL "F")
            math(EXPR _res "${_res} + 15")
        else()
            math(EXPR _res "${_res} + ${NIBBLE}")
        endif()

        # Update the length of HEX
        string(LENGTH "${HEX}" _strlen)
    endwhile()

    # Set the decimal result
    set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

# This function converts a string to a decimal value.
# Arguments:
#   STR: The string to be converted.
#   DEC: The variable to store the decimal value.
function(toDec STR DEC)
    # Convert the string to uppercase.
    string(TOUPPER "${STR}" STR)

    # Check if the string matches the pattern for a hexadecimal value.
    if (STR MATCHES "^0X[0-9A-F]+")
        # Convert the hexadecimal string to decimal.
        from_hex("${STR}" _res)
    else()
        # If the string is not a hexadecimal value, assign the original string to the result.
        set(_res "${STR}")
    endif()

    # Set the decimal value in the specified variable.
    set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

# Read the contents of the symbol file into a variable
file(READ ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.symbol SYMBOL_FILE)
# Use regex to match a specific pattern in the symbol file
string(REGEX MATCH
    "[0-9a-f]+[ \t]+[0-9a-f]+[ \t]+NOTYPE[ \t]+GLOBAL[ \t]+DEFAULT[ \t]+ABS[ \t]+__lcd_base_a"
    LCDA_ADDR
    ${SYMBOL_FILE}
)
#message("LCDA_ADDR=${LCDA_ADDR}")

# This code block performs a regex match on the LCDA_ADDR variable
string(REGEX MATCH
    "^[0-9a-f]+"
    LCDA_ADDR       # The variable to store the matched substring
    ${LCDA_ADDR}    # The input string to match against
)
#message("LCDA_ADDR=${LCDA_ADDR}")

# Converts the hex string "${LCDA_ADDR}" to its corresponding decimal value
from_hex("${LCDA_ADDR}" CFG_LCDA_ADDR)
#message("CFG_LCDA_ADDR=${CFG_LCDA_ADDR}")

if (CFG_LCD_INIT_ON_BOOTING)

    if (CMAKE_HOST_WIN32)
        # For Windows host, use the 'cmd' command to get the file size
        execute_process(COMMAND cmd /c for %I in (lcd_clear.bin) do @echo %~zI OUTPUT_VARIABLE CFG_LCD_CLEAR_FILESIZE)
    else()
        # For non-Windows host, use the 'stat' command to get the file size
        execute_process(COMMAND stat --format=%s lcd_clear.bin OUTPUT_VARIABLE CFG_LCD_CLEAR_FILESIZE)
    endif()
    #message("CFG_LCD_CLEAR_FILESIZE=${CFG_LCD_CLEAR_FILESIZE}")

    # Convert a bitmap image file to BMP format using ImageMagick's `convert` command
    execute_process(
        COMMAND convert
            # Define the BMP format as bmp3
            -define bmp:format=bmp3
            # Enable alpha channel for the BMP image
            -define bmp3:alpha=true
            # Path to the input bitmap image file
            ${PROJECT_SOURCE_DIR}/data/bitmap/${CFG_LCD_BOOT_BITMAP}
            # Path to the output BMP file
            ${CMAKE_CURRENT_BINARY_DIR}/bitmap.bmp
    )

    # Check the value of CFG_LCD_BPP and set the appropriate format option for dataconv
    if (CFG_LCD_BPP STREQUAL "4")
        set(args --format=ARGB8888) # Set the format option to ARGB8888
    else()
        set(args --format=RGB565)   # Set the format option to RGB565
    endif()

    # Check if CFG_LCD_BOOT_BITMAP_DITHER is defined and add the dither option to args if true
    if (DEFINED CFG_LCD_BOOT_BITMAP_DITHER)
        set(args ${args} --dither)  # Enable dithering
    endif()

    message("dataconv ${args}")

    # Execute the dataconv command with the specified arguments
    execute_process(
        COMMAND dataconv ${args} -r -q -b ${CMAKE_CURRENT_BINARY_DIR}/bitmap.bmp -o ${CMAKE_CURRENT_BINARY_DIR}/bitmap.raw
        OUTPUT_VARIABLE DATACONV_OUTPUT # Store the output of the command in DATACONV_OUTPUT
    )
    #message("DATACONV_OUTPUT=${DATACONV_OUTPUT}")

    # Extracts the second group of numbers from DATACONV_OUTPUT and stores it in BITMAP_WIDTH
    string(REGEX REPLACE "([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)" "\\2" BITMAP_WIDTH ${DATACONV_OUTPUT})
    # Extracts the third group of numbers from DATACONV_OUTPUT and stores it in CFG_LCD_BOOT_BITMAP_HEIGHT
    string(REGEX REPLACE "([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)" "\\3" CFG_LCD_BOOT_BITMAP_HEIGHT ${DATACONV_OUTPUT})
    #message("CFG_LCDA_ADDR=${CFG_LCDA_ADDR}")
    #message("BITMAP_WIDTH=${BITMAP_WIDTH}")
    #message("BITMAP_HEIGHT=${CFG_LCD_BOOT_BITMAP_HEIGHT}")

    # Calculate the width of the boot bitmap based on the bitmap width and the LCD BPP
    math(EXPR CFG_LCD_BOOT_BITMAP_WIDTH "${BITMAP_WIDTH} * ${CFG_LCD_BPP}")
    #message("CFG_LCD_BOOT_BITMAP_WIDTH=${CFG_LCD_BOOT_BITMAP_WIDTH}")

    # Calculate the address of the boot bitmap based on the LCD pitch, height, width, and BPP
    math(EXPR CFG_LCD_BOOT_BITMAP_ADDR "${CFG_LCD_PITCH} * ((${CFG_LCD_HEIGHT} - ${CFG_LCD_BOOT_BITMAP_HEIGHT}) >> 1) + ((${CFG_LCD_WIDTH} * ${CFG_LCD_BPP} - ${CFG_LCD_BOOT_BITMAP_WIDTH}) >> 1)")
    math(EXPR CFG_LCD_BOOT_BITMAP_ADDR "((${CFG_LCDA_ADDR} + ${CFG_LCD_BOOT_BITMAP_ADDR}) >> 2) << 2")
    #message("CFG_LCD_BOOT_BITMAP_ADDR=${CFG_LCD_BOOT_BITMAP_ADDR}")
else()
    # Check if the host is Windows
    if (CMAKE_HOST_WIN32)
        # Create an empty file called "bitmap.raw" using the "type nul" command
        execute_process(COMMAND cmd /c type nul OUTPUT_FILE bitmap.raw)
    else()
        # Create an empty file called "bitmap.raw" using the "touch" command
        execute_process(COMMAND touch bitmap.raw)
    endif()

    # Set the address, height, and width of the boot bitmap to 0
    set(CFG_LCD_BOOT_BITMAP_ADDR 0)
    set(CFG_LCD_BOOT_BITMAP_HEIGHT 0)
    set(CFG_LCD_BOOT_BITMAP_WIDTH 0)
endif()

if (NOT CFG_LCD_MULTIPLE AND CFG_LCD_ENABLE)
    configure_file(${PROJECT_SOURCE_DIR}/sdk/target/lcd/${CFG_LCD_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/lcd.txt)
endif()

configure_file(${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/ram.txt)

execute_process(COMMAND ${CPP}  -I${CMAKE_CURRENT_BINARY_DIR} -E -P -o ${CMAKE_CURRENT_BINARY_DIR}/init.scr ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/init.scr.in)
