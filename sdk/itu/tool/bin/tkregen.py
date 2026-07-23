# coding: utf-8
import os
import sys

import re
import time
import subprocess
import math
import errno

from os.path import isfile, isdir, join, getsize
#import platform

from tkinter import *
import tkinter as tk
import shutil

# pip install psutil
import psutil
import xml.etree.ElementTree as ET

###############################################################################
# defs
###############################################################################

def set_path_entry(text):
    value_text_entry.set(text)


def remove_file(file):
    try:
        os.remove(file)
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise print("Remove file error, {}".format(e))
            sys.exit()


# search
def search_files(path, type):
    result = []

    if os.path.exists(path):

        for root, dirs, files in os.walk(path):
            for f in files:
                f = os.path.join(root, f)

                file_name, ext_name = os.path.splitext(f)

                # remove "." of ext_name
                if ext_name.startswith(r'.'):
                    ext_name = ext_name[1:]

                if ext_name == type:
                    result.append(f)

    return result


# generate ite_regen.ini
def write_ini(file, tool_path):
    ini = str("itu_regen.ini")
    path = str(os.path.split(tool_path)[0])

    full_ini = path + os.sep + ini

    try:
        # delete exist ini.
        if os.path.exists(ini):
            remove_file(ini)

        with open(full_ini, "w+", encoding = 'UTF-8') as f:
            f.write(r'<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' + "\n")
            f.write(r'<ITURegenWork>' + "\n")
            f.write(r'    ' + '<JOB' + str(0) + ' XMLFILE="' +  file + "\" WORK=\"1\" JPEGQT=\"0\"/>\n")
            f.write(r'</ITURegenWork>' + "\n")
            f.close()
    except Exception as e:
        print("Write ini file error: {}.".format(e))

    return 0


# copy file
def copy_itu(file, dest):
    f  = os.path.abspath(file)
    try:
        if os.path.exists(file):
            d = os.path.dirname(f)
            d_re = re.compile(r"project(\\|\/)(.+$)")
            match = d_re.search(d)

            if (dest[-1:] == "\\") or (dest[-1:] == "/"):
                dn = dest + match.group(2)
            else:
                dn = dest + "/" +  match.group(2)

            if not os.path.exists(dn):
                os.makedirs(dn, mode = 0o777)
                #print("Create dir: " + dn)

            #print("f:  " + f)
            #print("dn: " + dn)
            #print("d:  " + d)

            if (d != dn):
                shutil.copy(f, dn)
            #else:
                #print("The file can not be copied on itself.")

    except Exception as e:
        print(f'copy_itu - Exception: {e}')
        pass
    #else:
    #    print(f'copy_itu - other file error: {e.strerror}')
    #    pass


# create a null size file.
def write_null_file(file, bit):
    result = bytearray()
    b = bytes()
    b = bit.to_bytes(bit, byteorder="little")
    #print(b)
    try:
        if os.path.exists(file):
            #print("Original file length: {}. ".format(len(result)))
            #print("Modified file length: {}. ".format(len(result)))

            with open(file, "wb+") as f:
                f.write(b)
        else:
            return 0
    except Exception as e:
        print("Error: " + e.strerror)


def remove_os_sep(path):
    result = path

    p_re = re.compile(r'^[\\+|\/+](.+)$')
    match = p_re.search(result)
    if match:
        result = match.group(1)

    p_re = re.compile(r'^(.+)[\\+|\/+]$')
    match = p_re.search(result)
    if match:
        result = match.group(1)

    return result


# calculate total file size in list
def get_total_size(list):
    result = 0

    for i in range(len(list)):
        if os.path.isfile(list[i]):
            result += math.ceil(getsize(list[i]) / 1024)

    return result


def fixed_length_string(text, length):
    result = text.strip()

    while len(result) < length:
        result = ' ' + result

    return result


# check the process is exist
def proc_exist(process_name):
    pl = psutil.pids()
    for pid in pl:
        if psutil.Process(pid).name() == process_name:
            return pid


def check_process_running(process_name):
    pids = psutil.pids()
    result = False

    for pid in pids:
        try:
            ps = psutil.Process(pid)
            name = ps.name()
            if process_name == name:
                result = True
        except psutil.NoSuchProcess:  # Catch the error caused by the process no longer existing
            result = False #"PROCESS_IS_NOT_RUNNING"
            pass  # Ignore it
        else:
            if process_name in name:
                #print(f"{name} running with pid: {pid}")
                result = True

    return result


# process regen
def regen_button_event():
    # delay start regen event
    delay_time = 0
    try:
        t = int(value_delay_entry.get())
        delay_time = t
    except ValueError as e:
        print(f'set delay time error: {str(e)}.')
        pass

    while delay_time > 0:
        time.sleep(1)
        delay_time -= 1

    regen_start_time = time.time()
    # init output log
    print("\r\n")
    result = []
    files  = []

    # get count of list box
    count = file_listbox.size()
    # append every item of list box into files list
    for i in range(0, count, 1):
        files.append(file_listbox.get(i))

    i = 0
    j = 0
    timeout_init = 120
    total_size = get_total_size(files)
    current_size = 0

    while len(files) > 0:
        try:
            # init files
            xml = files[i]
            itu = os.path.splitext(xml)[0] + r'.itu'

            # init size
            size_of_xml = 0
            size_of_itu = 0

            if os.path.exists(xml):
                if os.path.exists(regen_tool):
                    write_ini(xml, regen_tool)

                    try:
                        wait_factor = 0.5
                        while(check_process_running(tool_name)):
                            #print(f'{tool_name} is running.')
                            time.sleep(wait_factor)
                    except Exception as e:
                        print(f'Exception: waiting {tool_name} error: {str(e)} ')
                        pass

                    start_time = time.time()

                    if os.path.exists(xml):
                        # get xml size, KiBytes
                        size_of_xml = math.ceil(getsize(xml) / 1024)

                    current_size += size_of_xml
                    percent = current_size / total_size * 100
                    percent = fixed_length_string("{:3.2f} ".format(percent), 6)

                    # set time out seconds
                    timeout_seconds = math.ceil(size_of_xml / 290 + 16)

                    # for slow cpu
                    #timeout_seconds = math.ceil((size_of_xml / 290 + 16) * 1.125)
                    #timeout_seconds = math.ceil((size_of_xml / 290 + 16) * 1.25)

                    # multi processes in sand box
                    timeout_seconds = math.ceil((size_of_xml / 290 + 16) * 2.00) * 2

                    message = "[{}%] \t".format(percent)
                    sys.stdout.write(message)
                    sys.stdout.flush()

                    proc = subprocess.Popen([regen_tool, ''])

                    try:
                        # waiting for task
                        proc.wait(timeout_seconds)

                        remove_flag = True
                    except subprocess.TimeoutExpired as e:
                        remove_flag = False
                        # kill regen tool process
                        proc.kill()

                        current_size -= size_of_xml

                        # remove error item
                        itu_regen_ini = os.path.join(init_path, 'itu_regen.ini')
                        if os.path.exists(itu_regen_ini):
                            remove_file(itu_regen_ini)

                        #print("regen failed, waiting seconds after kill process")

                        print(f'regen failed, skipping {xml} : {str(e)}.')
                        # remove converted file
                        files.pop(i)
                        file_listbox.delete(i, END)

                        time.sleep(0)
                        pass

                    if remove_flag == True:
                        # remove converted file
                        files.pop(i)
                        file_listbox.delete(i, END)
                        #print("{} = {}".format(j, files[i]))

                        # copy itu files
                        copy_itu(itu, output_path)

                        # reset itu file
                        # write_null_file(itu, 0)

                    if os.path.exists(itu):
                        # get itu size, KiBytes
                        size_of_itu = math.ceil(getsize(itu) / 1024)

                    total_time = time.time() - start_time

                    xml_time = size_of_xml / total_time
                    itu_time = size_of_itu / total_time

                    filename = remove_os_sep(xml.replace(str(path_entry.get()), ''))

                    total_time  = fixed_length_string("{:3.1f}".format(total_time), 5)
                    size_of_xml = fixed_length_string(format(size_of_xml,','), 8)
                    size_of_itu = fixed_length_string(format(size_of_itu,','), 6)

                    message = total_time + "(s)\txml size=" + size_of_xml + " \titu size=" + size_of_itu + " \t{}".format(filename)
                    time.sleep(1)
                    print(message)
                else:
                    print(f'Not found \"{regen_tool}\".')
            else:
                print(f'Not found \"{xml}\".')

            j += 1
        except Exception as e:
            i += 1
            j += 1
            print(f'rengen failed [{str(e)}]: {xml}.')
            # skip error event
            pass

    total_time = time.time() - regen_start_time
    print("regen total time: {0:,.3f}secs ".format(total_time))

    return 0


# appending files into list box
def listbox_append(list, tk_listbox):
    for i in list:
        tk_listbox.insert("end", i)

    # get count of list box
    count = tk_listbox.size()
    # set selection from 0 ~ count - 1, select all
    tk_listbox.selection_set(0, count)
    return 0


# checking XML file format
def check_xml_type(file):
    result = False

    skip_xml = [r'customwidget.xml']

    f = os.path.basename(file)

    if f in skip_xml:
        return False
    elif os.path.exists(file):
        with open(file, 'r', encoding='utf-8') as f:
            text = str(f.read())
            text = text.strip()

            count_index = -1
            count_rindex = -1

            try:
                count_index  = text.index(r'<ITU>')
                count_rindex = text.rindex(r'</ITU>')
                distance = count_rindex - count_index + len(r'<ITU>')
            except ValueError as e:
                distance = -1

            f.close()

            if distance >= 0:
                result = True

    return result


def clean_xml_files(list):
    for i in range(len(list) - 1, -1, -1):
        if check_xml_type(list[i]) == False:
            list.remove(list[i])

    return list


def search_project_root(path):
    if os.path.exists(path):
        result = path
    else:
        # default project folder in sdk
        os.chdir(f'..{os.sep}..{os.sep}..{os.sep}..{os.sep}project')
        result = os.getcwd()

    # porject is exists
    try:
        return result
    except Exception as e:
        print(f'Write ini file error: {str(e)}.')
        pass
        return os.getcwd()


def check_xml_is_itu(xml_file, root_name):
    try:
        if os.path.exists(xml_file):
            tree = ET.parse(xml_file)
            root = tree.getroot()

            if root.tag == root_name:
                #print(f'root = {root.tag}')
                result = True
            else:
                result = False

    except Exception as e:
        print(f'open {xml_file} failed, {str(e)}')
        result = False
        pass

    return result


def search_button_event():
    # clean all items of tk.listbox
    file_listbox.delete(0, tk.END)

    # init varialbes
    file_type   = str(type_var.get())
    search_path = str(path_entry.get())

    # searching
    root = search_project_root(os.getcwd())

    # remove space
    search_path = search_path.strip()

    files = []

    if os.path.exists(search_path):
        files = search_files(search_path, file_type)
    elif search_path == "":

        # get current directory name
        dir = os.getcwd()
        while len(files) <= 10 or dir != root:
            files = search_files(dir, file_type)

            #21/10/25 checking xml type
            files = clean_xml_files(files)

            if len(files) > 10:
                set_path_entry(dir)
                break
            else:
                # get last directory name
                dir = str(os.path.abspath(os.path.dirname(os.getcwd())))
                os.chdir(dir)
                # get current directory
                dir = os.getcwd()

    # sorting file list
    files.sort()

    if file_type == 'itu':
        # rename .itu to .xml
        for i in range(len(files)):
            files[i] = files[i][:-4] + r'.xml'

        # check xml format
        files = clean_xml_files(files)

    for i in range(len(files) - 1, 0, -1):
        xml = files[i]
        #print(f'{xml}')
        if not check_xml_is_itu(xml, 'ITU'):
            #print(f'The root of {xml} is not "ITU".')
            files.pop(i)

    # append file(s) to list box
    listbox_append(files, file_listbox)

    return 0


def search_regen_tool(path, regen_tool_filename):
    result = ""

    # get current directory name
    dir = path
    while dir != "/" or dir != "\\":
        if os.path.exists(dir):
            for root, dirs, files in os.walk(dir):
                for f in files:
                    if f == regen_tool_filename:
                        result = os.path.join(root, f)
                        return result
                        break

        if result != "":
            break
        else:
            # get last directory name
            dir = str(os.path.abspath(os.path.dirname(os.getcwd())))
            os.chdir(dir)
            # get current directory name
            dir = os.getcwd()

    return result


def check_input_regex(value):
    result = 0

    p_re = re.compile(r'^(\d+)$')
    match = p_re.search(value)

    if match:
        result = int(match.group(1))

    return result


###############################################################################
#
# MAIN LOOP
#
###############################################################################

if __name__ == "__main__":
    tool_name  = 'DrawrockerGUIDesigner.exe'
    work_path   = os.path.dirname(sys.argv[0])
    w          = work_path[0:-16]
    output_path = f'{str(w)}regen_itu{os.sep}'

    if os.path.exists(output_path) and os.path.isdir(output_path):
        shutil.rmtree(output_path)
        print(f'rmtree {str(output_path)}')

    print(f'output_path: {str(output_path)}')

    # save original directory
    init_path = os.getcwd()

    # remove regen ini file
    remove_file("itu_regen.ini")

    regen_tool = search_regen_tool(os.getcwd(), tool_name)

    #sys.exit()

    window = tk.Tk()
    #window = ttk.Tk()
    window.title('itu regen')
    window.geometry('1280x800')

    # type
    type_var = tk.StringVar()

    # search xml or itu file(s)
    search_button = tk.Button(window, text='search', command = search_button_event)
    #search_button.grid(row = 0, column = 0)
    search_button.pack()

    regen_button = tk.Button(window, text='regen', command = regen_button_event)
    #regen_button.grid(row = 1, column = 0)
    regen_button.pack()


    # labeling text
    # path_label = tk.Label(window,               # window text
    #                  text  = 'searching path',  # display text
    #                  #bg    = '#EEBB00',        # background color
    #                  font  = ('Arial', 12),     # font and size
    #                  width = 15, height = 2)    # font width and height
    # path_label.pack()


    # set variable for update search path
    value_text_entry = tk.StringVar()

    # input area
    path_entry = tk.Entry(window,       # input
                        width = 120,
                        textvariable = value_text_entry)  # width
    path_entry.pack()


    radio_itu = tk.Radiobutton(window, text='itu', variable = type_var, value = "itu")
    radio_itu.pack()

    radio_xml = tk.Radiobutton(window, text='xml', variable = type_var, value = "xml")
    radio_xml.pack()
    # set default = xml type
    radio_xml.select()


    # delay seconds input seconds
    value_delay_entry = tk.StringVar()

    # delay seconds input area
    delay_entry = tk.Entry(window, width = 20, textvariable = value_delay_entry)

    delay_entry.insert(0, r'0')
    delay_entry.pack()

    # set variable for list box
    file_list = StringVar()

    file_ver_scrollbar = tk.Scrollbar(window, orient="vertical")
    file_ver_scrollbar.pack(side = "right", fill = "y")
    file_listbox = tk.Listbox(window, yscrollcommand = file_ver_scrollbar.set, selectmode = EXTENDED, listvariable = file_list)


    file_listbox.pack(side = "left", fill = "both", expand = True)
    file_ver_scrollbar.config(command = file_listbox.yview)

    # return tool path
    os.chdir(init_path)

    # init search path
    itu_path = init_path
    itu_path = search_project_root('')

    if os.path.exists(itu_path):
        set_path_entry(itu_path + os.sep)

    # auto regen or gui mode
    if len(sys.argv) == 1:
        window.mainloop()
    elif len(sys.argv) == 2:
        if sys.argv[1] == r'auto':
            search_button_event()
            regen_button_event()
        else:
            sys.exit()