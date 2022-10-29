"""
Serial Monitor

.setDTR(False)
https://stackoverflow.com/questions/15460865/disable-dtr-in-pyserial-from-code
"""

import tkinter as tk
from tkinter import *
from tkinter import ttk as ttk
from tkinter import scrolledtext as tkscroll
from tkinter import messagebox as msgbox
import serial
import serial.tools.list_ports as list_ports
import time
import struct
import json
import threading
import pickle
#from screeninfo import get_monitors

import random

class Drag_and_Drop_Listbox(tk.Listbox):
	""" A tk listbox with drag'n'drop reordering of entries. """
	def __init__(self, master, **kw):
		kw['selectmode'] = tk.EXTENDED
		kw['activestyle'] = 'none'
		tk.Listbox.__init__(self, master, kw)
		self.bind('<Button-1>', self.getState, add='+')
		self.bind('<Button-1>', self.setCurrent, add='+')
		self.bind('<B1-Motion>', self.shiftSelection)
		self.curIndex = None
		self.curState = None
	def setCurrent(self, event):
		''' gets the current index of the clicked item in the listbox '''
		self.curIndex = self.nearest(event.y)
	def getState(self, event):
		''' checks if the clicked item in listbox is selected '''
		i = self.nearest(event.y)
		self.curState = self.selection_includes(i)
	def shiftSelection(self, event):
		''' shifts item up or down in listbox '''
		i = self.nearest(event.y)
		if self.curState == 1:
			self.selection_set(self.curIndex)
		else:
			self.selection_clear(self.curIndex)
		if i < self.curIndex:
			# Moves up
			x = self.get(i)
			selected = self.selection_includes(i)
			self.delete(i)
			self.insert(i+1, x)
			if selected:
				self.selection_set(i+1)
			self.curIndex = i
		elif i > self.curIndex:
			# Moves down
			x = self.get(i)
			selected = self.selection_includes(i)
			self.delete(i)
			self.insert(i-1, x)
			if selected:
				self.selection_set(i-1)
			self.curIndex = i

class JobSetupCanvas(tk.Canvas):
	def __init__(self, master, option):
		self.master = master
		scroller = tk.Scrollbar(self)
	def addJobCanv(self):
		addJob()

class AddJobDlg(tk.Toplevel):
	jobCbo = None
	jobCanvas = None
	def __init__(self, title, ico = None):
		self.title(title) #Add Job
		if ico:
			self.iconphoto(False, ico)
		jobLbl = tk.Label(self, text='Jobs:').grid(row=0, column=0, padx=0, pady=12, sticky=tk.NE)
		self.jobCbo = ttk.Combobox(self, width=12)
		self.jobCbo.grid(row=0, column=1, padx=12, pady=12, sticky=tk.NE)
		self.jobCbo['values'] = COMMAND_TYPES
		self.jobCbo.set('Select Job')
		self.jobCbo.bind('<<ComboboxSelected>>', self.changeCanvas())
		self.jobCanvas = JobSetupCanvas(self, -1)
		okBtn = tk.Button(self, text='OK', width=10, command=addJob).grid(row=3, column=1, padx=0, pady=12, sticky=tk.S)
		cancelBtn = tk.Button(self, text='Cancel', width=10, command=self.destroy())
		cancelBtn.grid(row=3, column=3, padx=0, pady=12, sticky=tk.S)
		self.bind('<Return>', self.addJobDlg())
		self.bind('<Escape>', self.destroy())
		self.update()
		dw = self.winfo_width()
		dh = self.winfo_height()
		self.minsize(dw, dh)
		self.maxsize(dw, dh)
		self.resizable(0, 0)
		self.grab_set()
		cancelBtn.focus_set()
	def changeCanvas(self):
		self.jobCanvas = JobSetupCanvas(self, self.jobCbo.current())
	def addJobDlg(self):
		self.jobCanvas.addJobCanv()

def get_str_of_chr(chr_in_byte):
	cd = ord(chr_in_byte)
	if 0x20 <= cd and cd <= 0x7e or 0xa1 <= cd:
		if cd == 92:
			return '\\\\'
		return chr(cd)
	else:
		if cd == 9:
			return '\t'
		elif cd == 10:
			return '\n'
		elif cd == 13:
			return '\\r'
	return '\\x{:02x}'.format(cd)

def get_hexstr_of_bytes(bytes_in):
	hex_txt = ''
	for b in struct.unpack(str(len(bytes_in)) + 'c', bytes_in):
		hex_txt += get_hexstr_of_chr(b)
	return hex_txt

def get_hexstr_of_chr(chr_in_byte):
	cd = ord(chr_in_byte)
	st = '{:02X}'.format(cd)
	if cd == 10:
		st += '\n'
	else:
		st += ' '
	return st

def is_hex_digit(a_byte):
	return b'0' <= a_byte and a_byte <= b'9' or b'A' <= a_byte and a_byte <= b'F' or b'a' <= a_byte and a_byte <= b'f'

def is_oct_digit(a_byte):
	return b'0' <= a_byte and a_byte <= b'7'

def decode_esc(str_of_chr):
	sbs = bytes([ord(c) for c in str_of_chr])
	dbs = b''
	err = None
	idx = 0
	while idx < len(sbs):
		by = sbs[idx:idx+1]
		if by == b'\\':
			idx += 1
			by = sbs[idx:idx+1]
			if by == b'\\' or by == b"'" or by == b'"':
				dbs += by
			elif by == b'0' and not is_oct_digit(sbs[idx+1:idx+2]):
				dbs += b'\0'
			elif by == b'a':
				dbs += b'\a'
			elif by == b'b':
				dbs += b'\b'
			elif by == b't':
				dbs += b'\t'
			elif by == b'n':
				dbs += b'\n'
			elif by == b'v':
				dbs += b'\v'
			elif by == b'f':
				dbs += b'\f'
			elif by == b'r':
				dbs += b'\r'
			elif by == b'x':
				if is_hex_digit(sbs[idx+1:idx+2]):
					if is_hex_digit(sbs[idx+2:idx+3]):
						dbs += bytes([int(sbs[idx+1:idx+3], 16)])
						idx += 3
						continue
				err = {'from': idx-1, 'to': idx+3, 'msg': f'Value Error: invalid {str_of_chr[idx-1:idx+3]} escape at position {idx-1}'}
				break
			elif is_oct_digit(by):
				od = 1
				if is_oct_digit(sbs[idx+1:idx+2]):
					od += 1
					if is_oct_digit(sbs[idx+2:idx+3]):
						od += 1
				ov = int(sbs[idx:idx+od], 8)
				if ov > 255:
					od -= 1
					ov >>= 3
				dbs += bytes([ov])
				idx += od
				continue
			else:
				if by:
					ch = chr(ord(by))
					to = idx + 1
				else:
					ch = ''
					to = idx
				err = {'from': idx-1, 'to': to, 'msg': f"Syntax Error: invalid escape sequence '\\{ch}' at position {idx-1}"}
				break
		else:
			dbs += by
		idx += 1
	return dbs, err

def sendCmd(event):
	global sentTexts, sentTextsPtr, lineEnding
	txt = str(txText.get())
	lst = len(sentTexts)
	if txt != '':
		bs, err = decode_esc(txt)
		if err:
			writeConsoles(err['msg'] + '\n', 2)
			txText.xview(err['from'])
			txText.selection_range(err['from'], err['to'])
			txText.icursor(err['to'])
			return
		if lst > 0 and sentTexts[lst-1] != txt or lst == 0:
			sentTexts.append(txt)
		sentTextsPtr = len(sentTexts)
		if lineEnding == 1:
			bs += b'\n'
		elif lineEnding == 2:
			bs += b'\r'
		elif lineEnding == 3:
			bs += b'\r\n'
		currentPort.write(bs)
		if showSentTextVar.get():
			if dispHexVar.get():
				txt = ''.join([get_hexstr_of_chr(bytes([i])) for i in bs])
			else:
				txt = ''.join([get_str_of_chr(bytes([i])) for i in bs])
			writeConsoles(txt, 1)
		txText.delete(0, tk.END)

def sendSerial(text, log):
	global lineEnding
	if text != '':
		bs, err = decode_esc(text)
		if err:
			writeConsoles(err['msg'] + '\n', 2)
			txText.xview(err['from'])
			txText.selection_range(err['from'], err['to'])
			txText.icursor(err['to'])
			return
		if lineEnding == 1:
			bs += b'\n'
		elif lineEnding == 2:
			bs += b'\r'
		elif lineEnding == 3:
			bs += b'\r\n'
		currentPort.write(bs)
		if log:
			if dispHexVar.get():
				txt = ''.join([get_hexstr_of_chr(bytes([i])) for i in bs])
			else:
				txt = ''.join([get_str_of_chr(bytes([i])) for i in bs])
			writeConsoles(txt, 1)

def upKeyCmd(event):
	global sentTextsPtr, lastTxText
	if sentTextsPtr == len(sentTexts):
		lastTxText = str(txText.get())
	if sentTextsPtr > 0:
		sentTextsPtr -= 1
		txText.delete(0, tk.END)
		txText.insert(tk.END, sentTexts[sentTextsPtr])

def downKeyCmd(event):
	global sentTextsPtr
	if sentTextsPtr < len(sentTexts):
		sentTextsPtr += 1
		txText.delete(0, tk.END)
		if sentTextsPtr == len(sentTexts):
			txText.insert(tk.END, lastTxText)
		else:
			txText.insert(tk.END, sentTexts[sentTextsPtr])

def changePort(newport, event):
	global portDesc
	disableSending()
	if currentPort.is_open: #only if open; add connectPort, disconnectPort; update button
		closePort()
		#writeConsoles(portDesc + ' disconnected.\n', 2)
		currentPort.port = newport
		try:
			portDesc = ports[currentPort.port]
		except KeyError:
			return
		writeConsoles('Connecting to ' + portDesc + '...', 2)
		root.update()
		try:
			currentPort.open()
		except:
			if currentPort.port is None:
				writeConsoles('Select a port to connect to!\n', 2)
			elif currentPort.port not in ports:
				writeConsoles(str(currentPort.port) + ' is unavailable!\n', 2)
			else:
				writeConsoles('Connection failed!\n', 2)
			#currentPort.port = None
		if currentPort.is_open:
			enableSending()
			rxPolling()
			writeConsoles('Connected.\n', 2)
			sendSerial(chr(133), False)
			#root.after(50, )
	else:
		currentPort.port = newport
		try:
			portDesc = ports[currentPort.port]
		except KeyError:
			return

def connectPort(): #getStates, getCurrentPos
	global portDesc
	disableSending()
	if currentPort.is_open: #only if open; add connectPort, disconnectPort; update button
		closePort()
	else:
		if currentPort.port is None:
				writeConsoles('Select a port to connect to!\n', 2)
		elif portDesc != '':
			writeConsoles('Connecting to ' + portDesc + '...', 2)
		else:
			writeConsoles('Connecting to ' + str(currentPort.port) + '...', 2)
		try:
			currentPort.open()
		except:
			if currentPort.port not in ports:
				writeConsoles(str(currentPort.port) + ' is unavailable!\n', 2)
			else:
				writeConsoles('Connection failed!\n', 2)
			#currentPort.port = None
		if currentPort.is_open:
			enableSending()
			rxPolling()
			writeConsoles('Connected.\n', 2)
			sendSerial(chr(133), False)

def clearOutputCmd():
	global isEndByNL, isEndByNL2, isEndByNL3, lastUpdatedBy
	rxText.configure(state=tk.NORMAL)
	rxText.delete('1.0', tk.END)
	rxText.configure(state=tk.DISABLED)
	sentText.configure(state=tk.NORMAL)
	sentText.delete('1.0', tk.END)
	sentText.configure(state=tk.DISABLED)
	logText.configure(state=tk.NORMAL)
	logText.delete('1.0', tk.END)
	logText.configure(state=tk.DISABLED)
	isEndByNL = True
	isEndByNL2 = True
	isEndByNL3 = True
	lastUpdatedBy = 2

def showTxTextMenu(event):
	if txText.selection_present():
		sta=tk.NORMAL
	else:
		sta=tk.DISABLED
	for i in range(2):
		txTextMenu.entryconfigure(i, state=sta)
	try:
		root.clipboard_get()
		txTextMenu.entryconfigure(2, state=tk.NORMAL)
	except:
		txTextMenu.entryconfigure(2, state=tk.DISABLED)
	try:
		txTextMenu.tk_popup(event.x_root, event.y_root)
	finally:
		txTextMenu.grab_release()

def showRxTextMenu(event):
	if len(rxText.tag_ranges(tk.SEL)):
		rxTextMenu.entryconfigure(0, state=tk.NORMAL)
	else:
		rxTextMenu.entryconfigure(0, state=tk.DISABLED)
	# if currentPort.isOpen():
	# 	rxTextMenu.entryconfigure(2, state=tk.NORMAL)
	# else:
	# 	rxTextMenu.entryconfigure(2, state=tk.DISABLED)
	try:
		rxTextMenu.tk_popup(event.x_root, event.y_root)
	finally:
		rxTextMenu.grab_release()

def showJobLstBoxMenu(event):
	if jobLstBox.curselection():
		jobLstBoxMenu.entryconfigure(0, state=tk.NORMAL)
		jobLstBoxMenu.entryconfigure(1, state=tk.NORMAL)
		if len(jobClipboard):
			jobLstBoxMenu.entryconfigure(2, state=tk.NORMAL)
		else:
			jobLstBoxMenu.entryconfigure(2, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(4, state=tk.NORMAL)
		jobLstBoxMenu.entryconfigure(5, state=tk.NORMAL)
		jobLstBoxMenu.entryconfigure(6, state=tk.NORMAL)
		jobLstBoxMenu.entryconfigure(8, state=tk.NORMAL)
		jobLstBoxMenu.entryconfigure(9, state=tk.NORMAL)
	else:
		jobLstBoxMenu.entryconfigure(0, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(1, state=tk.DISABLED)
		if len(jobClipboard):
			jobLstBoxMenu.entryconfigure(2, state=tk.NORMAL)
		else:
			jobLstBoxMenu.entryconfigure(2, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(4, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(5, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(6, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(8, state=tk.DISABLED)
		jobLstBoxMenu.entryconfigure(9, state=tk.DISABLED)
	try:
		jobLstBoxMenu.tk_popup(event.x_root, event.y_root)
	finally:
		rxTextMenu.grab_release()

def writeConsoles(txt, upd=0, replace=True):
	global isEndByNL, isEndByNL2, isEndByNL3
	if txt == '':
		return
	tm = ''
	ad = ''
	if showTimestampVar.get():
		tm = time.strftime('%H:%M:%S.{}'.format(repr(time.time()).split('.')[1][:3]))
		ad = tm + ' >> '
	ad += txt
	if replace:
		ad = ad.replace('\r', '\n')
	if not upd:
		if not isEndByNL:
			ad = '\n' + ad
		rxText.configure(state=tk.NORMAL)
		rxText.insert(tk.END, ad)
		if autoscrollVar.get():
			rxText.see(tk.END)
		rxText.configure(state=tk.DISABLED)
		if txt[-1] == '\n':
			isEndByNL = True
		else:
			isEndByNL = False
	elif upd == 1:
		if not isEndByNL2:
			ad = '\n' + ad
		sentText.configure(state=tk.NORMAL)
		sentText.insert(tk.END, ad)
		if autoscrollVar.get():
			sentText.see(tk.END)
		sentText.configure(state=tk.DISABLED)
		if txt[-1] == '\n':
			isEndByNL2 = True
		else:
			isEndByNL2 = False
	elif upd == 2:
		if not isEndByNL3:
			ad = '\n' + ad
		logText.configure(state=tk.NORMAL)
		logText.insert(tk.END, ad)
		if autoscrollVar.get():
			logText.see(tk.END)
		logText.configure(state=tk.DISABLED)
		if txt[-1] == '\n':
			isEndByNL3 = True
		else:
			isEndByNL3 = False
	else:
		return

	lastUpdatedBy = upd

def rxPolling():
	global isJogging, lineEnding
	if not currentPort.is_open:
		return
	#preset = time.perf_counter_ns()
	try:
		while currentPort.in_waiting > 0:
			ch = currentPort.read_until()
			txt = ch.decode('ASCII')
			if txt[0] == '#':
				quickParseState(txt)
			elif txt[0] == '@':
				quickParsePos(txt)
			elif txt[0] == '!':
				quickParseError(txt)	
			#https://github.com/gnea/grbl/wiki/Grbl-v1.1-Jogging
			#elif txt[0] == '?':
			#elif txt[0] == '%':
			#elif txt[0] == '*':
			#elif txt[0] == '~':
			elif receivingFunction is None:
				if txt != "&\n":
					if dispHexVar.get():
						txt = get_hexstr_of_bytes(ch)
					writeConsoles(txt)
			else:
				receivingFunction(txt)
	except serial.SerialException as se:
		closePort()
		msgbox.showerror(APP_TITLE, "Couldn't access the {} port".format(portDesc))
	root.after(10, rxPolling) # polling in 10ms interval

def listPortsPolling():
	global ports
	ps = {p.name: p.description for p in list_ports.comports()}
	pn = sorted(ps)
	if pn != sorted(ports):
		# if len(ports) == 0: # if no port before
		# 	enableSending()
		# elif len(pn) == 0: # now if no port
		# 	disableSending()
		ports = ps
	root.after(1000, listPortsPolling) # polling every 1 second

def disableSending():
	connectBtn.configure(text='Connect')
	sendBtn['state'] = tk.DISABLED
	txText.unbind('<Return>')
	runBtn['state'] = tk.DISABLED
	pauseBtn['state'] = tk.DISABLED
	stopBtn['state'] = tk.DISABLED
	goBtn['state'] = tk.DISABLED
	#getBtn['state'] = tk.DISABLED
	#getSettingBtn['state'] = tk.DISABLED#
	#defaultBtn['state'] = tk.DISABLED#
	#cancelBtn['state'] = tk.DISABLED#
	#sendSettingBtn['state'] = tk.DISABLED#
	lcValveBtn['state'] = tk.DISABLED
	fcValveBtn['state'] = tk.DISABLED
	wash1Btn['state'] = tk.DISABLED
	wash2Btn['state'] = tk.DISABLED
	rmTipBtn['state'] = tk.DISABLED
	goHomeBtn['state'] = tk.DISABLED
	parkBtn['state'] = tk.DISABLED
	cycleStartBtn['state'] = tk.DISABLED
	feedHoldBtn['state'] = tk.DISABLED
	skipJobBtn['state'] = tk.DISABLED
	clearErrBtn['state'] = tk.DISABLED
	resetBtn['state'] = tk.DISABLED
	objSpn['state'] = tk.DISABLED
	teachZeroBtn['state'] = tk.DISABLED
	teachOffsetBtn['state'] = tk.DISABLED
	teachPenetrationBtn['state'] = tk.DISABLED
	y_negBtn['state'] = tk.DISABLED
	x_negBtn['state'] = tk.DISABLED
	x_posBtn['state'] = tk.DISABLED
	y_posBtn['state'] = tk.DISABLED
	z_negBtn['state'] = tk.DISABLED
	z_posBtn['state'] = tk.DISABLED
	a_posBtn['state'] = tk.DISABLED
	a_negBtn['state'] = tk.DISABLED
	y_negBtn.unbind('<ButtonPress-1>')
	x_negBtn.unbind('<ButtonPress-1>')
	x_posBtn.unbind('<ButtonPress-1>')
	y_posBtn.unbind('<ButtonPress-1>')
	z_negBtn.unbind('<ButtonPress-1>')
	z_posBtn.unbind('<ButtonPress-1>')
	a_posBtn.unbind('<ButtonPress-1>')
	a_negBtn.unbind('<ButtonPress-1>')
	y_negBtn.unbind('<ButtonRelease-1>')
	x_negBtn.unbind('<ButtonRelease-1>')
	x_posBtn.unbind('<ButtonRelease-1>')
	y_posBtn.unbind('<ButtonRelease-1>')
	z_negBtn.unbind('<ButtonRelease-1>')
	z_posBtn.unbind('<ButtonRelease-1>')
	a_posBtn.unbind('<ButtonRelease-1>')
	a_negBtn.unbind('<ButtonRelease-1>')
	stateLbl.configure(text='Not Connected')
	asstateLbl.configure(text='Not Connected')
	xCurrLbl.configure(text='-')
	yCurrLbl.configure(text='-')
	zCurrLbl.configure(text='-')
	aCurrLbl.configure(text='-')

def enableSending():
	connectBtn.configure(text='Disconnect')
	sendBtn['state'] = tk.NORMAL
	txText.bind('<Return>', sendCmd)
	runBtn['state'] = tk.NORMAL
	pauseBtn['state'] = tk.NORMAL
	stopBtn['state'] = tk.NORMAL
	goBtn['state'] = tk.NORMAL
	#getBtn['state'] = tk.NORMAL
	#getSettingBtn['state'] = tk.NORMAL#
	#defaultBtn['state'] = tk.NORMAL#
	#cancelBtn['state'] = tk.NORMAL#
	#sendSettingBtn['state'] = tk.NORMAL#
	lcValveBtn['state'] = tk.NORMAL
	fcValveBtn['state'] = tk.NORMAL
	wash1Btn['state'] = tk.NORMAL
	wash2Btn['state'] = tk.NORMAL
	rmTipBtn['state'] = tk.NORMAL
	goHomeBtn['state'] = tk.NORMAL
	parkBtn['state'] = tk.NORMAL
	cycleStartBtn['state'] = tk.NORMAL
	feedHoldBtn['state'] = tk.NORMAL
	skipJobBtn['state'] = tk.NORMAL
	clearErrBtn['state'] = tk.NORMAL
	resetBtn['state'] = tk.NORMAL
	objSpn['state'] = tk.NORMAL
	teachZeroBtn['state'] = tk.NORMAL
	teachOffsetBtn['state'] = tk.NORMAL
	teachPenetrationBtn['state'] = tk.NORMAL
	y_negBtn['state'] = tk.NORMAL
	x_negBtn['state'] = tk.NORMAL
	x_posBtn['state'] = tk.NORMAL
	y_posBtn['state'] = tk.NORMAL
	z_negBtn['state'] = tk.NORMAL
	z_posBtn['state'] = tk.NORMAL
	a_posBtn['state'] = tk.NORMAL
	a_negBtn['state'] = tk.NORMAL
	y_negBtn.bind('<ButtonPress-1>', lambda x:startJog('Y-'))
	x_negBtn.bind('<ButtonPress-1>', lambda x:startJog('X-'))
	x_posBtn.bind('<ButtonPress-1>', lambda x:startJog('X'))
	y_posBtn.bind('<ButtonPress-1>', lambda x:startJog('Y'))
	z_negBtn.bind('<ButtonPress-1>', lambda x:startJog('Z-'))
	z_posBtn.bind('<ButtonPress-1>', lambda x:startJog('Z'))
	a_posBtn.bind('<ButtonPress-1>', lambda x:startJog('A'))
	a_negBtn.bind('<ButtonPress-1>', lambda x:startJog('A-'))
	y_negBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	x_negBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	x_posBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	y_posBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	z_negBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	z_posBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	a_posBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	a_negBtn.bind('<ButtonRelease-1>', emptyRecFuncJog)
	stateLbl.configure(text='Connected')
	asstateLbl.configure(text='Connected')

def closePort():
	if currentPort.is_open:
		currentPort.close()
		if portDesc != '':
			writeConsoles(portDesc + ' disconnected.\n', 2)
		else: 
			writeConsoles(str(currentPort.port) + ' disconnected.\n', 2)
		disableSending()

def startJog(axis):
	global receivingFunction, jogText 
	if receivingFunction is not None:
		writeConsoles('Error: cannot execute jog (busy)', 2)
		return
	jogText = '$J=G22G91'+axis+'400'+'F'+'{:.2f}'.format(as_settings.max_rate[0]*goSpeedVal.get()/100)#axis! 0.100
	receivingFunction = lambda feedback:runJog(feedback)
	#
	for i in range(5):#send 5 jogText
		sendSerial(jogText, False)

def runJog(feedback):
	global receivingFunction, jogText 
	if feedback == '&\n':
		sendSerial(jogText, False)
	elif feedback != 'ok\n':
		writeConsoles(feedback, 0)
		emptyRecFuncJog()

def emptyRecFuncJog(event = None):
	sendSerial('^', False)
	emptyRecFunc()

def emptyRecFunc(event = None):
	global receivingFunction
	receivingFunction = None



def sendTeach(teachtype):
	sendSerial('$T'+objNumVar.get()+teachtype, True)

def GoCoord(event = None):
	if receivingFunction is not None:
		writeConsoles('Error: cannot execute motion (busy)', 2)
		return
	command = ''
	rate = 0.0
	if goRelAbsVal.get() == 1:
		command += 'G91'
	else:
		command += 'G90'
	if coordUnitVal.get() == 1:
		command += 'G22'
	else:
		command += 'G21'
	movex = movexText.get()
	if movex and movex != '.' and movex != '-':
		command += 'X'
		command += movex
		rate = as_settings.max_rate[0]
	movey = moveyText.get()
	if movey and movey != '.' and movey != '-':
		command += 'Y'
		command += movey
		rate = as_settings.max_rate[1]
	movez = movezText.get()
	if movez and movez != '.' and movez != '-':
		command += 'Z'
		command += movez
		rate = as_settings.max_rate[2]
	movea = moveaText.get()
	if movea and movea != '.' and movea != '-':
		command += 'A'
		command += movea
		rate = as_settings.max_rate[3]
	if not command:
		return
	command += 'F' + '{:.2f}'.format(rate*goSpeedVal.get()/100)
	sendSerial(command, True)

def quickParseState(state):
	state = state.replace('#', '')
	states = state.split()
	sys_txt = as_txt = ''
	if int(states[0]) == 0:
		sys_txt = SYS_STATES[0]
	else:
		for b in range(7):
			if int(states[0]) & (1 << b):
				if sys_txt:
					sys_txt += ', '
				sys_txt = sys_txt + SYS_STATES[b+1]
	if int(states[1]) == 0:
		as_txt = SYS_STATES[0]
	else:
		for b in range(7):
			if int(states[1]) & (1 << b):
				if as_txt:
					as_txt += ', '
				as_txt = as_txt + AS_STATES[b+1]
	stateLbl.configure(text=sys_txt)
	asstateLbl.configure(text=as_txt)

def quickParsePos(pos):
	pos = pos.replace('@', '')
	coords = pos.split()
	xCurrLbl.configure(text=coords[0].split('|')[coordUnitVal.get()])
	yCurrLbl.configure(text=coords[1].split('|')[coordUnitVal.get()])
	zCurrLbl.configure(text=coords[2].split('|')[coordUnitVal.get()])
	aCurrLbl.configure(text=coords[3].split('|')[coordUnitVal.get()])

def quickParseError(err):
	err = err.replace('!', '')
	err = err.replace('\n', '')
	writeConsoles('AS Error: '+ERROR_DICT[err], 2)

def GetCoord(event=None): #on mc_line end receive coord
	global receivingFunction
	if receivingFunction is not None:
		writeConsoles('Error: cannot get coordinates (busy)', 2)
		return
	receivingFunction = lambda feedback:ParseCoord(feedback)
	sendSerial('$DP', False)

def ParseCoord(coords): #set, cr/lf
	emptyRecFunc()
	coords = coords.replace(')', '')
	lines = coords.split("\r")
	for l in lines:
		data = l.split("=")
		if len(data) == 2:
			if data[0] == 'AXIS1':
				xCurrLbl.configure(text=data[1].split('(')[goRelAbsVal.get()])
			elif data[0] == 'AXIS2':
				yCurrLbl.configure(text=data[1].split('(')[goRelAbsVal.get()])
			elif data[0] == 'AXIS3':
				zCurrLbl.configure(text=data[1].split('(')[goRelAbsVal.get()])
			elif data[0] == 'AXIS4':
				aCurrLbl.configure(text=data[1].split('(')[goRelAbsVal.get()])

def GetState(event=None): #on state change receive!!!
	global receivingFunction
	if receivingFunction is not None:
		#writeConsoles('Error: cannot get coordinates (busy)', 2)
		return
	receivingFunction = lambda feedback:ParseState(feedback)
	sendSerial('$DS', False)

def ParseState(state): #set, cr/lf
	emptyRecFunc()
	lines = state.split("\r")
	sys_txt = as_txt = ''
	for l in lines:
		data = l.split("=")
		if len(data) == 2:
			if data[0] == 'sys':
				for b in range(8):
					if int(data[1]) & 1 << b:
						if sys_txt:
							sys_txt += ', '
						sys_txt += SYS_STATES[b+1]
			elif data[0] == 'AS':
				for b in range(8):
					if int(data[1]) & 1 << b:
						if as_txt:
							as_txt += ', '
						as_txt += AS_STATES[b+1]
	stateLbl.configure(text=sys_txt)
	asstateLbl.configure(text=as_txt)

def GetSettings():
	global receivingFunction
	if receivingFunction is not None:
		return
	receivingFunction = lambda feedback:ParseSettings(feedback)
	sendSerial('$$', False)

def ParseSettings(settings): #!!!,set, cr/lf
	emptyRecFunc()

def GetComponent(comp_no):
	global receivingFunction
	if receivingFunction is not None:
		writeConsoles('Error: cannot request component (busy)', 2)
		return
	elif 1 > comp_no or comp_no > 12:
		return
	receivingFunction = lambda feedback:ParseComponent(feedback)
	sendSerial('$DC'+str(comp_no), False)

def ParseComponent(comp): #!!!,set, cr/lf
	emptyRecFunc()

def GetSyringe():
	global receivingFunction
	if receivingFunction is not None:
		writeConsoles('Error: cannot request syringe (busy)', 2)
		return
	receivingFunction = lambda feedback:ParseSyringe(feedback)
	sendSerial('$DN', False)

def ParseSyringe(syr): #set, cr/lf, update display
	emptyRecFunc()
	lines = syr.split("\r")
	for l in lines:
		data = l.split("=")
		if len(data) == 2:
			if data[0] == 'flg':
				current_syringe.flags = int(data[1])
			elif data[0] == 'ndl.offs.(mm)':
				current_syringe.needle_offset = float(data[1])
			elif data[0] == 'max.vol(uL)':
				current_syringe.max_volume = float(data[1])
			elif data[0] == 'vol/step(uL/step)':
				current_syringe.volume_per_step = float(data[1])
			elif data[0] == 'overfill':
				current_syringe.overfill = float(data[1])

def clearEntry(event=None):
	movexText.delete(0,END)
	moveyText.delete(0,END)
	movezText.delete(0,END)
	moveaText.delete(0,END)


def isMoveValid(entry): #delete . - on field exit
	if entry == '' or entry == '-':
		return True
	elif coordUnitVal.get():
		try:
			int(entry)
		except ValueError:
			return False
		return True
	elif not coordUnitVal.get():
		if entry == '.':
			return True
		try:
			float(entry)
		except ValueError:
			return False
		return True
	return False

def validatePosFloat(entry):
	if '-' in entry:
		return False
	if entry == '' or entry == '.':
		return True
	try:
		val = float(entry)
		if val <= 0:
			return False
	except ValueError:
		return False
	return True

def validateRatio(entry):
	if '-' in entry:
		return False
	if entry == '' or entry == '.':
		return True
	try:
		ratio = float(entry)
		if ratio < 0 or ratio > 1:
			return False
	except ValueError:
		return False
	return True

def validate_uint8(entry):
	if '-' in entry:
		return False
	if entry == '':
		return True
	try:
		val = int(entry)
		if val <= 0 or val >= pow(2,8):
			return False
	except ValueError:
		return False
	return True

def validatePulse_us(entry):
	if '-' in entry:
		return False
	if entry == '':
		return True
	try:
		val = int(entry)
		if val < 3 or val >= pow(2,8):
			return False
	except ValueError:
		return False
	return True


def showAbout():
	msgbox.showinfo(APP_TITLE, 'AS FC @University of Debrecen, 2021')

def exitRoot():
	global lineEnding
	data = {}
	data['autoscroll'] = autoscrollVar.get()
	data['showtimestamp'] = showTimestampVar.get()
	data['showsenttext'] = showSentTextVar.get()
	data['displayhex'] = dispHexVar.get()
	data['lineending'] = lineEnding
	data['baudrateindex'] = BAUD_RATES.index(currentPort.baudrate)
	data['databits'] = currentPort.bytesize
	data['parity'] = currentPort.parity
	data['stopbits'] = currentPort.stopbits
	data['readtimeout'] = currentPort.timeout
	data['bytetimeout'] = currentPort.inter_byte_timeout
	data['comport'] = currentPort.port
	data['goRelAbs'] = goRelAbsVal.get()
	data['coordUnit'] = coordUnitVal.get()
	data['portlist'] = ports
	with open(fname+'.json', 'w') as jfile:
		json.dump(data, jfile, indent=4)
		jfile.close()
	closePort()
	root.destroy()

def as_setting(): #set lastget (or default) values OR empty?
	if receivingFunction is not None:
		writeConsoles('Error: cannot edit settings (busy)', 2)
		return
	ASsettingDlg = tk.Toplevel()
	ASsettingDlg.title('Autosampler Settings')
	if ico:
		ASsettingDlg.iconphoto(False, ico)
	for i in range(20):
		tk.Grid.rowconfigure(ASsettingDlg, i, weight=1)
		tk.Grid.columnconfigure(ASsettingDlg, i, weight=1)

	steps_per_mm_Label = tk.Label(ASsettingDlg, text="step/unit:").grid(row=1, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	max_travel_Label = tk.Label(ASsettingDlg, text="Max Travel:").grid(row=2, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	max_rate_Label = tk.Label(ASsettingDlg, text="Max Rate:").grid(row=3, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	acceleration_Label = tk.Label(ASsettingDlg, text="Acceleration:").grid(row=4, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	ratio_Label = tk.Label(ASsettingDlg, text="Ratio:").grid(row=5, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	steps_per_mm_Entry = max_travel_Entry = max_rate_Entry = acceleration_Entry = ratio_Entry = []
	for i in range(len(AXIS_NAMES)):
		tk.Label(ASsettingDlg, text=AXIS_NAMES[i]).grid(row=0, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		new_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
		new_Entry.grid(row=1, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		steps_per_mm_Entry.append(new_Entry)
		new_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
		new_Entry.grid(row=2, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		max_travel_Entry.append(new_Entry)
		new_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
		new_Entry.grid(row=3, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		max_rate_Entry.append(new_Entry)
		new_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
		new_Entry.grid(row=4, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		acceleration_Entry.append(new_Entry)
		new_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
		new_Entry.grid(row=5, column=i*4+4, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
		ratio_Entry.append(new_Entry)
	step_invert_mask_Chk = dir_invert_mask_Chk = homing_dir_mask_Chk = sig_out_invert_mask_Chk = sig_in_invert_mask_Chk = limit_invert_mask_Chk = []
	step_invert_mask_Label = tk.Label(ASsettingDlg, text="Step Invert Mask:").grid(row=7, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	dir_invert_mask_Label = tk.Label(ASsettingDlg, text="Dir Invert Mask:").grid(row=8, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_dir_mask_Label = tk.Label(ASsettingDlg, text="Homing Dir Mask:").grid(row=9, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	sig_out_invert_mask_Label = tk.Label(ASsettingDlg, text="Sig. Out Invert Mask:").grid(row=10, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	sig_in_invert_mask_Label = tk.Label(ASsettingDlg, text="Sig. In Invert Mask:").grid(row=11, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	limit_invert_mask_Label = tk.Label(ASsettingDlg, text="Limit Invert Mask:").grid(row=12, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	for i in range(len(AXIS_NAMES)):
		tk.Label(ASsettingDlg, text=AXIS_NAMES[i]).grid(row=6, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=step_invert_mask_Var[i])
		new_Chk.grid(row=7, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		step_invert_mask_Chk.append(new_Chk)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=dir_invert_mask_Var[i])
		new_Chk.grid(row=8, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		dir_invert_mask_Chk.append(new_Chk)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=homing_dir_mask_Var[i])
		new_Chk.grid(row=9, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		homing_dir_mask_Chk.append(new_Chk)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=sig_out_invert_mask_Var[i])
		new_Chk.grid(row=10, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		sig_out_invert_mask_Chk.append(new_Chk)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=sig_in_invert_mask_Var[i])
		new_Chk.grid(row=11, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		sig_in_invert_mask_Chk.append(new_Chk)
		new_Chk = tk.Checkbutton(ASsettingDlg, variable=limit_invert_mask_Var[i])
		new_Chk.grid(row=12, column=i+16, padx=0, pady=8, sticky=tk.S+tk.EW)
		limit_invert_mask_Chk.append(new_Chk)

	homing_feed_rate_Label = tk.Label(ASsettingDlg, text="Homing Feed Rate:").grid(row=7, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_feed_rate_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	homing_feed_rate_Entry.grid(row=7, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_seek_rate_Label = tk.Label(ASsettingDlg, text="Homing Seek Rate:").grid(row=8, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_seek_rate_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	homing_seek_rate_Entry.grid(row=8, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_pulloff_Label = tk.Label(ASsettingDlg, text="Homing Pulloff:").grid(row=9, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_pulloff_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	homing_pulloff_Entry.grid(row=9, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	rpm_max_Label = tk.Label(ASsettingDlg, text="RPM max:").grid(row=10, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	rpm_max_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	rpm_max_Entry.grid(row=10, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	rpm_min_Label = tk.Label(ASsettingDlg, text="RPM min:").grid(row=11, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	rpm_min_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	rpm_min_Entry.grid(row=11, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	junction_deviation_Label = tk.Label(ASsettingDlg, text="Junction Deviation:").grid(row=13, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	junction_deviation_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	junction_deviation_Entry.grid(row=13, column=16, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	arc_tolerance_Label = tk.Label(ASsettingDlg, text="Arc Tolerance:").grid(row=14, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	arc_tolerance_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(posFloatValidation, '%P'))
	arc_tolerance_Entry.grid(row=14, column=16, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	pulse_microseconds_Label = tk.Label(ASsettingDlg, text="Pulse Microseconds:").grid(row=12, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	pulse_microseconds_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(pulse_us_Validation, '%P'))
	pulse_microseconds_Entry.grid(row=12, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	stepper_idle_lock_time_Label = tk.Label(ASsettingDlg, text="Stepper Idle Lock Time:").grid(row=13, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	stepper_idle_lock_time_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(uint8_Validation, '%P'))
	stepper_idle_lock_time_Entry.grid(row=13, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_debounce_delay_Label = tk.Label(ASsettingDlg, text="Debounce Delay:").grid(row=14, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_debounce_delay_Entry = tk.Entry(ASsettingDlg, width=8, validate='all', validatecommand=(uint8_Validation, '%P'))
	homing_debounce_delay_Entry.grid(row=14, column=7, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)

	homing_Label = tk.Label(ASsettingDlg, text="Homing").grid(row=15, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	homing_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[4])
	homing_Chk.grid(row=15, column=4, padx=0, pady=8, sticky=tk.S+tk.EW)
	hard_limit_Label = tk.Label(ASsettingDlg, text="Hard Limit").grid(row=15, column=6, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	hard_limit_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[3])
	hard_limit_Chk.grid(row=15, column=10, padx=0, pady=8, sticky=tk.S+tk.EW)
	soft_limit_Label = tk.Label(ASsettingDlg, text="Soft Limit").grid(row=15, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	soft_limit_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[5])
	soft_limit_Chk.grid(row=15, column=16, padx=0, pady=8, sticky=tk.S+tk.EW)
	invert_limits_Label = tk.Label(ASsettingDlg, text="Invert Limits").grid(row=16, column=0, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	invert_limits_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[6])
	invert_limits_Chk.grid(row=16, column=4, padx=0, pady=8, sticky=tk.S+tk.EW)
	invert_probe_Label = tk.Label(ASsettingDlg, text="Invert Probe").grid(row=16, column=6, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	invert_probe_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[7])
	invert_probe_Chk.grid(row=16, column=10, padx=0, pady=8, sticky=tk.S+tk.EW)
	laser_mode_Label = tk.Label(ASsettingDlg, text="Laser Mode").grid(row=16, column=12, columnspan=4, padx=4, pady=8, sticky=tk.S+tk.EW)
	laser_mode_Chk = tk.Checkbutton(ASsettingDlg, variable=as_setting_flag_Var[1])
	laser_mode_Chk.grid(row=16, column=16, padx=0, pady=8, sticky=tk.S+tk.EW)

	if currentPort.is_open:
		btn_state = tk.NORMAL
	else:
		btn_state = tk.DISABLED
	sendSettingBtn = tk.Button(ASsettingDlg, width=10, state=btn_state, text='Set', command=editJob).grid(row=17, column=2, columnspan=3, padx=4, pady=25)
	getSettingBtn = tk.Button(ASsettingDlg, width=10, state=btn_state, text='Get', command=GetSettings).grid(row=17, column=5, columnspan=3, padx=4, pady=25)
	defaultBtn = tk.Button(ASsettingDlg, width=10, state=btn_state, text='Default', command=editJob).grid(row=17, column=8, columnspan=3, padx=4, pady=25)
	clearBtn = tk.Button(ASsettingDlg, width=10, state=btn_state, text='Clear', command=editJob).grid(row=17, column=11, columnspan=3, padx=4, pady=25)
	cancelBtn = tk.Button(ASsettingDlg, width=10, state=btn_state, text='Cancel', command=editJob)
	cancelBtn.grid(row=17, column=14, columnspan=3, padx=4, pady=4)

	ASsettingDlg.bind('<Return>', setPort)
	ASsettingDlg.bind('<Escape>', lambda x:ASsettingDlg.destroy())
	ASsettingDlg.update()
	rw = root.winfo_width()
	rh = root.winfo_height()
	rx = root.winfo_rootx()
	ry = root.winfo_rooty()
	dw = ASsettingDlg.winfo_width()
	dh = ASsettingDlg.winfo_height()
	ASsettingDlg.geometry(f'{dw}x{dh}+{rx+int((rw-dw)/2)}+{ry+int((rh-dh)/2)}')
	ASsettingDlg.minsize(dw, dh)
	ASsettingDlg.maxsize(dw, dh)
	ASsettingDlg.resizable(0, 0)
	ASsettingDlg.grab_set()
	cancelBtn.focus_set()

def setting():
	global settingDlg, portCbo, lineEndingCbo, baudrateCbo, dataBitsCbo, parityCbo, stopBitsCbo
	settingDlg = tk.Toplevel()
	settingDlg.title('Serial Settings')
	if ico:
		settingDlg.iconphoto(False, ico)

	tk.Label(settingDlg, text='COM Port:').grid(row=0, column=0, padx=0, pady=12, sticky=tk.NE)
	tk.Label(settingDlg, text='Line Ending:').grid(row=1, column=0, padx=0, pady=0, sticky=tk.NE)
	tk.Label(settingDlg, text='Baud Rate:').grid(row=2, column=0, padx=0, pady=12, sticky=tk.NE)
	portCbo = ttk.Combobox(settingDlg, width=12)
	portCbo.grid(row=0, column=1, padx=12, pady=12, sticky=tk.NE)
	portCbo['values'] = sorted(ports)
	if len(ports) > 0:
		try:
			portCbo.current(sorted(ports).index(currentPort.port))
		except (ValueError, TypeError) as e:
			portCbo['state'] = 'readonly'
			if currentPort.port is None:
				portCbo.set('Select port')
			else:
				portCbo.set(str(currentPort.port))
	else:
		portCbo['state'] = tk.DISABLED
		portCbo.set('No port')

	lineEndingCbo = ttk.Combobox(settingDlg, width=12, state='readonly')
	lineEndingCbo.grid(row=1, column=1, padx=12, pady=0, sticky=tk.NE)
	lineEndingCbo['values'] = ('No line ending', 'Newline', 'Carriage return', 'Both CR & NL')
	lineEndingCbo.current(lineEnding)
	baudrateCbo = ttk.Combobox(settingDlg, width=12, state='readonly')
	baudrateCbo.grid(row=2, column=1, padx=12, pady=12, sticky=tk.NE)
	baudrateCbo['values'] = list(str(b) + ' baud' for b in BAUD_RATES)
	baudrateCbo.current(BAUD_RATES.index(currentPort.baudrate))

	tk.Label(settingDlg, text='Data bits:').grid(row=0, column=2, padx=0, pady=12, sticky=tk.NE)
	tk.Label(settingDlg, text='Parity:').grid(row=1, column=2, padx=0, pady=0, sticky=tk.NE)
	tk.Label(settingDlg, text='Stop bits:').grid(row=2, column=2, padx=0, pady=12, sticky=tk.NE)
	dataBitsCbo = ttk.Combobox(settingDlg, width=10, state='readonly')
	dataBitsCbo.grid(row=0, column=3, padx=12, pady=12, sticky=tk.NE)
	dataBitsCbo['values'] = DATABITS
	dataBitsCbo.set(currentPort.bytesize)
	parityCbo = ttk.Combobox(settingDlg, width=10, state='readonly')
	parityCbo.grid(row=1, column=3, padx=12, pady=0, sticky=tk.NE)
	parityCbo['values'] = PARITY_VAL
	parityCbo.current(PARITY.index(currentPort.parity))
	stopBitsCbo = ttk.Combobox(settingDlg, width=10, state='readonly')
	stopBitsCbo.grid(row=2, column=3, padx=12, pady=12, sticky=tk.NE)
	stopBitsCbo['values'] = STOPBITS
	stopBitsCbo.set(currentPort.stopbits)

	timeoutVar.set(str(currentPort.timeout))
	intertimeoutVar.set(str(currentPort.inter_byte_timeout))
	tk.Label(settingDlg, text='Read Timeout (s):').grid(row=0, column=4, padx=0, pady=12, sticky=tk.NE)
	tk.Label(settingDlg, text='Byte Timeout (ms):').grid(row=1, column=4, padx=0, pady=0, sticky=tk.NE)
	timeoutEntry = tk.Entry(settingDlg, width=10, textvariable=timeoutVar, validate='all', validatecommand=(posFloatValidation, '%P'))
	timeoutEntry.grid(row=0, column=5, padx=12, pady=12, sticky=tk.NE)
	intertimeoutEntry = tk.Entry(settingDlg, width=10, textvariable=intertimeoutVar, validate='all', validatecommand=(posFloatValidation, '%P'))
	intertimeoutEntry.grid(row=1, column=5, padx=12, pady=0, sticky=tk.NE)

	autoscrollCbt = tk.Checkbutton(settingDlg, text='Autoscroll', variable=autoscrollVar, onvalue=True, offvalue=False)
	autoscrollCbt.grid(row=0, column=6, padx=12, pady=12, sticky=tk.NW)
	showTimestampCbt = tk.Checkbutton(settingDlg, text='Show timestamp', variable=showTimestampVar, onvalue=True, offvalue=False)
	showTimestampCbt.grid(row=1, column=6, padx=12, pady=0, sticky=tk.NE)

	tk.Button(settingDlg, text='Default', width=10, command=defaultSetting).\
		grid(row=3, column=2, padx=0, pady=12, sticky=tk.S)
	tk.Button(settingDlg, text='OK', width=10, command=lambda:setPort(None)).\
		grid(row=3, column=1, padx=0, pady=12, sticky=tk.S)
	cancelBtn = tk.Button(settingDlg, text='Cancel', width=10, command=settingDlg.destroy)
	cancelBtn.grid(row=3, column=3, padx=0, pady=12, sticky=tk.S)
	settingDlg.bind('<Return>', setPort)
	settingDlg.bind('<Escape>', lambda x:settingDlg.destroy())
	settingDlg.update()
	rw = root.winfo_width()
	rh = root.winfo_height()
	rx = root.winfo_rootx()
	ry = root.winfo_rooty()
	dw = settingDlg.winfo_width()
	dh = settingDlg.winfo_height()
	settingDlg.geometry(f'{dw}x{dh}+{rx+int((rw-dw)/2)}+{ry+int((rh-dh)/2)}')
	settingDlg.minsize(dw, dh)
	settingDlg.maxsize(dw, dh)
	settingDlg.resizable(0, 0)
	settingDlg.grab_set()
	cancelBtn.focus_set()

def defaultSetting():
	baudrateCbo.set(115200)
	lineEndingCbo.current(2)
	dataBitsCbo.set(serial.EIGHTBITS)
	parityCbo.current(PARITY.index(serial.PARITY_NONE))
	stopBitsCbo.set(serial.STOPBITS_ONE)
	timeoutVar.set('20.0')
	intertimeoutVar.set('0.5')

def setPort(event):
	global lineEnding
	if portCbo.get() != currentPort.port:
		changePort(portCbo.get(), 0)
	currentPort.parity = PARITY[parityCbo.current()]
	lineEnding = lineEndingCbo.current()
	currentPort.baudrate = BAUD_RATES[baudrateCbo.current()]

	currentPort.bytesize = DATABITS[dataBitsCbo.current()]
	currentPort.parity = PARITY[parityCbo.current()]
	currentPort.stopbits = STOPBITS[stopBitsCbo.current()]

	currentPort.timeout = float(timeoutVar.get())
	currentPort.inter_byte_timeout = float(intertimeoutVar.get())
	settingDlg.destroy()

class AS_settings(object):
	disable_reset=1 #uint8_t
	sig_out_invert_mask=0 #uint8_t
	sig_in_invert_mask=0 #uint8_t
	limit_invert_mask=0 #uint8_t
	# Axis settings
	steps_per_mm = [400,400,400,400,1]
	max_rate = [1000000,1000000,1000000,1000000,1000000]
	acceleration = [180000,180000,180000,180000,180000]
	max_travel = [400.0,200.0,200.0,100.0,360.0]

	# Remaining Grbl settings
	pulse_microseconds=10 #uint8_t
	step_invert_mask=0 #uint8_t
	dir_invert_mask=0 #uint8_t
	stepper_idle_lock_time=254 #uint8_t
	status_report_mask=1 #uint8_t
	junction_deviation=0.02
	arc_tolerance=0.002

	rpm_max=12000.0
	rpm_min=550.0

	flags=0 #uint8_t

	homing_dir_mask=0b1 #uint8_t
	homing_feed_rate=60.0
	homing_seek_rate=500.0
	homing_debounce_delay=250 #uint16_t
	homing_pulloff=5.0

	ratio = [0.5,1.0,1.0,1.0,1.0]

class AS_object(object):
	def __init__(self, settype):
		if isinstance(settype, int) and 0 <= settype < len(COMPONENT_TYPES):
			self.type = settype
	def __str__(self):
		full_line = self.name + '(' + COMPONENT_TYPES[self.type] + ')'
		return full_line
	type = 0
	flags = 0
	name = ''
	coordinate_zero = [0.0 for i in range(3)]
	offset = [0.0 for i in range(3)]
	dimensions = [0 for i in range(2)]

class AS_syringe(object):
	def __init__(self, flags):
		if isinstance(flags, int):
			self.type = flags
	def __str__(self):
		if self.name == '':
			return ''
		full_line = self.name + ' ' + str(self.max_volume) + 'uL, '
		for i in range(1,8):
			if self.flags & 1 << i:
				full_line += ', ' + SYRINGE_FLAGS_T[i-1]
			elif SYRINGE_FLAGS_F[i-1] != '':
				full_line += ', ' + SYRINGE_FLAGS_F[i-1]
		return full_line
	flags = 0
	name = ''
	needle_offset = 0
	max_volume = 0
	volume_per_step = 0
	overfill = 0

class Job(object):
	def __init__(self, settype):
		if isinstance(settype, int) and 0 <= settype < len(COMMAND_TYPES):
			self.type = settype
	def __str__(self):
		full_line = COMMAND_TYPES[self.type]
		return full_line
	type = -1
	source_comp = 0
	target_comp = 0
	source_x = 0
	source_y = 0
	target_x = 0
	target_y = 0
	needle_penetration = 0 #max?
	volume = 0
	air_volume = 0
	speed = 50 #default
	puncture_speed = speed/50
	volume_speed = 20
	delay_ms = 50
	cycles = 1
	getremove = 0

def refreshJobLstBox():
	jobLstBox.delete(0,END)
	for obj in jobLst:
		jobLstBox.insert(END, obj)
		# ha... color
		# ha repeat - indent
		# repeat: loop x, increment VIAL/VOLUME/SPEED

def addJob():
	#open type select window
	#oped edit window
	jobLst.append(Job(0)) #insert
	refreshJobLstBox()

def editJob(): #ok, cancel, exit
	index = jobLstBox.curselection()
	#oped edit window
	jobLst[index[0]].ax = 1 #try
	refreshJobLstBox()

def deleteJob():
	index = jobLstBox.curselection()
	offset = 0
	for i in index:
		jobLst.pop(i-offset)
		offset += 1
	refreshJobLstBox()

def clearJobs():
	jobLst.clear()
	jobLstBox.delete(0,END)

def moveUpJob():
	index = jobLstBox.curselection()
	offset = 0
	for i in index:
		if i-offset > 0:
			jobLst.insert(i-1, jobLst.pop(i))
		offset += 1
	refreshJobLstBox()
	offset = 0
	for i in index:
		if i-offset > 0:
			jobLstBox.selection_set(i-1, None)
		else:
			jobLstBox.selection_set(i, None)
		offset += 1

def moveDownJob():
	index = jobLstBox.curselection()
	index2 = index
	offset = 1
	for i in reversed(index2):
		if i < len(jobLst)-offset:
			jobLst.insert(i+1, jobLst.pop(i))
		offset += 1
	refreshJobLstBox()
	offset = 0
	for i in index:
		if i < len(jobLst)-len(index)+offset:
			jobLstBox.selection_set(i+1, None)
		else:
			jobLstBox.selection_set(i, None)
		offset += 1

def moveTopJob():
	index = jobLstBox.curselection()
	target = 0
	for i in index:
		jobLst.insert(target, jobLst.pop(i))
		target += 1
	refreshJobLstBox()
	jobLstBox.selection_set(0, target-1)

def moveBottomJob():
	index = jobLstBox.curselection()
	offset=0
	for i in index:
		jobLst.append(jobLst.pop(i-offset))
		offset += 1
	refreshJobLstBox()
	joblen = len(jobLst)
	jobLstBox.selection_set(joblen-offset, joblen-1)

def cutJob():
	jobClipboard.clear()
	index = jobLstBox.curselection()
	offset=0
	for i in index: #reverse
		jobClipboard.append(jobLst.pop(i-offset))
		offset += 1
	refreshJobLstBox()

def copyJob():
	jobClipboard.clear()
	index = jobLstBox.curselection()
	for i in index:
		jobClipboard.append(jobLst[i])
	refreshJobLstBox()

def pasteJob():
	index = jobLstBox.curselection()
	if len(index):
		i = index[0]
		for job in jobClipboard:
			jobLst.insert(i, job)
			i += 1
	else:
		for job in jobClipboard:
			jobLst.append(job)
	refreshJobLstBox()

if __name__ == '__main__':
	APP_TITLE = 'Autosampler Controller'
	AXIS_NAMES = ('X', 'Y', 'Z', 'A')
	COMMAND_TYPES = ('Transfer Liquid', 'Mix', 'Inject', 'Get/Remove Tip', 'Needle Wash', 'Syringe Purge', 'Go Home', 'REPEAT', 'COMMENT', 'GCODE')
	COMMAND_TYPES_GRBL = ('$T', '$M', '$I', '$R', '$W', '$W', '$MH', '', '', '')
	COMPONENT_TYPES = ('Vial Holder', 'Fraction Collector','Tip Holder', 'Tip Remover', 'Injector', 'Wash Station', 'Waste')
	SYS_STATES = ('Idle', 'ALARM', 'Check Mode', 'Homing', 'Cycle', 'Hold', 'Jog', 'Safety Stop', 'Sleep')
	AS_STATES = ('Idle', 'Park', 'Teach', 'Collect', '', '', '', '', '')
	SYRINGE_FLAGS_T = ('with Tip', 'with loop', 'Plastic', 'Luer', 'Lock', 'Extended')
	SYRINGE_FLAGS_F = ('', '', 'Glass','RN', '', '')
	# "~"-cycle start/resume; "!"-feed hold; 0x84-safety door; 0x85-jog cancel
	BAUD_RATES = (300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 76800, 115200, 230400, 500000, 1000000, 2000000)
	DATABITS = (serial.FIVEBITS, serial.SIXBITS, serial.SEVENBITS, serial.EIGHTBITS)
	PARITY = (serial.PARITY_EVEN, serial.PARITY_ODD, serial.PARITY_NONE, serial.PARITY_MARK, serial.PARITY_SPACE)
	PARITY_VAL = ('Even', 'Odd', 'None', 'Mark', 'Space')
	STOPBITS = (serial.STOPBITS_ONE, serial.STOPBITS_ONE_POINT_FIVE, serial.STOPBITS_TWO)
	LINE_ENDINGS = ('', '\n', '\r', '\r\n')
	ERROR_DICT = {
		'1': 'EXPECTED_COMMAND_LETTER',
		'2': 'BAD_NUMBER_FORMAT',
		'3': 'INVALID_STATEMENT',
		'4': 'NEGATIVE_VALUE',
		'5': 'SETTING_DISABLED',
		'6': 'SETTING_STEP_PULSE_MIN',
		'7': 'SETTING_READ_FAIL',
		'8': 'IDLE_ERROR',
		'9': 'SYSTEM_GC_LOCK',
		'10': 'SOFT_LIMIT_ERROR',
		'11': 'OVERFLOW',
		'12': 'MAX_STEP_RATE_EXCEEDED',
		'13': 'CHECK_DOOR',
		'14': 'LINE_LENGTH_EXCEEDED',
		'15': 'TRAVEL_EXCEEDED',
		'16': 'INVALID_JOG_COMMAND',
		'17': 'SETTING_DISABLED_LASER',
		'20': 'UNSUPPORTED_COMMAND',
		'21': 'MODAL_GROUP_VIOLATION',
		'22': 'UNDEFINED_FEED_RATE',
		'23': 'COMMAND_VALUE_NOT_INTEGER',
		'24': 'AXIS_COMMAND_CONFLICT',
		'25': 'WORD_REPEATED',
		'26': 'NO_AXIS_WORDS',
		'27': 'INVALID_LINE_NUMBER',
		'28': 'VALUE_WORD_MISSING',
		'29': 'UNSUPPORTED_COORD_SYS',
		'30': 'G53_INVALID_MOTION_MODE',
		'31': 'AXIS_WORDS_EXIST',
		'32': 'NO_AXIS_WORDS_IN_PLANE',
		'33': 'INVALID_TARGET',
		'34': 'ARC_RADIUS_ERROR',
		'35': 'NO_OFFSETS_IN_PLANE',
		'36': 'UNUSED_WORDS',
		'37': 'G43_DYNAMIC_AXIS_ERROR',
		'38': 'MAX_VALUE_EXCEEDED',
		'50': 'COORDINATES_OUT_OF_RANGE',
		'51': 'COMPONENT_OUT_OF_RANGE',
		'52': 'SETTING_OUT_OF_RANGE',
		'53': 'REQUESTED_COMPONENT_TYPE_NOT_INSTALLED',
		'54': 'OUT_OF_COLLECTION_VIALS'
	}
	ports = {p.name: p.description for p in list_ports.comports()}
	currentPort = serial.Serial(port=None, baudrate=115200, timeout=0.02, write_timeout=0, inter_byte_timeout=0.5)
	portDesc = ''
	sentTexts = []
	sentTextsPtr = 0
	lineEnding = 0
	isJogging = False
	jogText = ''
	isEndByNL = True
	isEndByNL2 = True
	isEndByNL3 = True
	lastUpdatedBy = 2
	jobLst = []
	jobClipboard = []
	receivingFunction = None
	as_settings = AS_settings()
	current_syringe = AS_syringe(0)
	saved_syringes = []
	saved_components = []
# for get_string in listbox:
#     for obj in my_list:
#         if get_string == obj.__str__():
#             print(f' You Selected {obj}')

	data = {}
	fname = 'settings' #__file__.rsplit('.', 1)[0] #
	jfile = None
	try:
		jfile = open(fname+'.json')
		data = json.load(jfile)
	except FileNotFoundError as fnfe:
		pass
	if jfile:
		jfile.close()
	compFile = 'comp'
	jfile = None
	try:
		jfile = open(compFile+'.asd', 'rb')
		saved_components = pickle.load(jfile)
	except FileNotFoundError as fnfe:
		pass
	if jfile:
		jfile.close()
	syrFile = 'syr'
	jfile = None
	try:
		jfile = open(syrFile+'.asd', 'rb')
		saved_syringes = pickle.load(jfile)
	except FileNotFoundError as fnfe:
		pass
	if jfile:
		jfile.close()

	root = tk.Tk()
	autoscrollVar = tk.BooleanVar()
	showTimestampVar = tk.BooleanVar()
	showSentTextVar = tk.BooleanVar()
	dispHexVar = tk.BooleanVar()
	goRelAbsVal = tk.IntVar()
	coordUnitVal = tk.BooleanVar()
	goSpeedVal = tk.IntVar()
	timeoutVar = tk.StringVar()
	intertimeoutVar = tk.StringVar()
	objNumVar = tk.StringVar()
	compIdVar = tk.StringVar()
	compInstVar = tk.BooleanVar()
	syrInstVar = tk.BooleanVar()
	step_invert_mask_Var = []
	dir_invert_mask_Var = []
	homing_dir_mask_Var = []
	sig_out_invert_mask_Var = []
	sig_in_invert_mask_Var = []
	limit_invert_mask_Var = []
	as_setting_flag_Var = []
	for i in range(4):
		step_invert_mask_Var.append(tk.BooleanVar())
		dir_invert_mask_Var.append(tk.BooleanVar())
		homing_dir_mask_Var.append(tk.BooleanVar())
		sig_out_invert_mask_Var.append(tk.BooleanVar())
		sig_in_invert_mask_Var.append(tk.BooleanVar())
		limit_invert_mask_Var.append(tk.BooleanVar())
		as_setting_flag_Var.append(tk.BooleanVar())
		as_setting_flag_Var.append(tk.BooleanVar())

	showSentTextVar.set(True)
	goRelAbsVal.set(0)
	coordUnitVal.set(1)
	goSpeedVal.set(75)

	di = data.get('goRelAbs')
	if di != None:
		goRelAbsVal.set(di)
	di = data.get('coordUnit')
	if di != None:
		coordUnitVal.set(di)

	di = data.get('baudrateindex')
	if di != None:
		currentPort.baudrate = BAUD_RATES[di]
	else:
		currentPort.baudrate = BAUD_RATES[4]

	di = data.get('lineending')
	if di != None:
		lineEnding = di
	else:
		lineEnding = 1

	di = data.get('displayhex')
	if di != None:
		dispHexVar.set(di)
	di = data.get('databits')
	if di != None:
		currentPort.bytesize = di
	di = data.get('parity')
	if di != None:
		currentPort.parity = di
	di = data.get('stopbits')
	if di != None:
		currentPort.stopbits = di
	di = data.get('readtimeout')
	if di != None:
		currentPort.timeout = di
	di = data.get('bytetimeout')
	if di != None:
		currentPort.inter_byte_timeout = di
	di = data.get('autoscroll')
	if di != None:
		autoscrollVar.set(di)
	di = data.get('showtimestamp')
	if di != None:
		showTimestampVar.set(di)
	di = data.get('comport')
	if di != None:
		currentPort.port=di
		try:
			portDesc = ports[currentPort.port]
		except (KeyError) as e:
			portDesc = ''#writeConsoles(str(currentPort.port) + ' is unavailable.', 2)
	else:
		currentPort.port=None
	"""	di = data.get('portindex')
 	if data.get('portlist') == ports:
		try:
			currentPort.port=sorted(ports)[di]
			portDesc = ports[currentPort.port]
		except (ValueError, TypeError, KeyError) as e:
			currentPort.port=None
	else:
		currentPort.port=None """

	root.title(APP_TITLE)
	ico = runPic = pausePic = stopPic = XupPic = YupPic = ZupPic = AupPic = XdownPic = YdownPic = ZdownPic = AdownPic = topPic = upPic = downPic = bottomPic = tk.PhotoImage()
	try:
		ico = tk.PhotoImage(file = './icons/appicon.png') #fname+'.png'
		runPic = tk.PhotoImage(file = './icons/run.png')
		pausePic = tk.PhotoImage(file = './icons/pause.png')
		stopPic = tk.PhotoImage(file = './icons/stop.png')
		topPic = tk.PhotoImage(file = './icons/top.png')
		upPic = tk.PhotoImage(file = './icons/up.png')
		downPic = tk.PhotoImage(file = './icons/down.png')
		bottomPic = tk.PhotoImage(file = './icons/bottom.png')

		AupPic = tk.PhotoImage(file = './icons/Aup.png')

		YdownPic = tk.PhotoImage(file = './icons/Ydown.png')
		ZdownPic = tk.PhotoImage(file = './icons/Zdown.png')
		AdownPic = tk.PhotoImage(file = './icons/Adown.png')

		XupPic = tk.PhotoImage(file = './icons/Xup.png')
		YupPic = tk.PhotoImage(file = './icons/Yup.png')
		ZupPic = tk.PhotoImage(file = './icons/Zup.png')

		XdownPic = tk.PhotoImage(file = './icons/Xdown.png')
	except:
		pass
	if ico:
		root.iconphoto(False, ico)
	root.protocol("WM_DELETE_WINDOW", exitRoot)

	# aspect_ratio = 16/9
	# for m in get_monitors():
	# 	if m.is_primary and m.width > 0 and m.height > 0:
	# 		aspect_ratio = m.width/m.height

	tk.Grid.columnconfigure(root, 0, weight=1, minsize=300) #14 37
	tk.Grid.columnconfigure(root, 1, weight=0) #10 33
	tk.Grid.columnconfigure(root, 2, weight=0) #10 33
	tk.Grid.columnconfigure(root, 3, weight=2, minsize=240) #14 32

	tk.Grid.rowconfigure(root, 0, weight=1)
	tk.Grid.rowconfigure(root, 1, weight=1)
	tk.Grid.rowconfigure(root, 2, weight=0)
	tk.Grid.rowconfigure(root, 3, weight=0)

	moveValidation=root.register(isMoveValid)
	posFloatValidation=root.register(validatePosFloat)
	uint8_Validation=root.register(validate_uint8)
	ratioValidation=root.register(validateRatio)
	pulse_us_Validation=root.register(validatePulse_us)

	menubar = tk.Menu(root)
	filemenu = tk.Menu(menubar)
	filemenu.add_command(label="Open")
	filemenu.add_command(label="Save")
	filemenu.add_command(label="Exit")
	menubar.add_cascade(label="File", menu=filemenu)
	root.config(menu=menubar)

	jobFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	jobFrame.grid(column=0, row=0, padx=3, pady=3, sticky=tk.NSEW)
	controlFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	controlFrame.grid(column=1, row=0, padx=3, pady=3, sticky=tk.NSEW)
	settingsFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	settingsFrame.grid(column=2, row=0, padx=3, pady=3, sticky=tk.NSEW)
	settingsNtb = ttk.Notebook(settingsFrame, width=300)
	settingsNtb.grid(column=0, row=0, padx=2, pady=3, sticky=tk.NS)
	compFrame = ttk.Frame(settingsNtb)
	syrFrame = ttk.Frame(settingsNtb)#
	txFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	txFrame.grid(column=3, row=0, padx=3, pady=3, sticky=tk.NSEW)
	logFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	logFrame.grid(column=0, row=1, rowspan=2, padx=3, pady=3, sticky=tk.NSEW)
	commandsFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	commandsFrame.grid(column=1, row=1, padx=3, pady=3, sticky=tk.NSEW)
	jogFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	jogFrame.grid(column=2, row=1, padx=3, pady=3, sticky=tk.NSEW)
	jogBtnFrame = tk.Frame(jogFrame)
	jogBtnFrame.grid(column=0, row=1, padx=3, pady=3, columnspan=10, sticky=tk.NSEW)
	rxFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	rxFrame.grid(column=3, row=1, rowspan=2, padx=3, pady=3, sticky=tk.NSEW)
	serialFrame = tk.Frame(root, borderwidth=1, relief=RIDGE)
	serialFrame.grid(column=1, row=2, columnspan=2, padx=3, pady=3, sticky=tk.NSEW)
	statusbar = tk.Frame(root, borderwidth=2, relief=RAISED)
	statusbar.grid(column=0, row=3, columnspan=4, sticky=tk.NSEW)
	for i in range(14):
		tk.Grid.rowconfigure(jobFrame, i, weight=1)
		tk.Grid.columnconfigure(jobFrame, i, weight=1)
		tk.Grid.rowconfigure(controlFrame, i, weight=1)
		tk.Grid.columnconfigure(controlFrame, i, weight=1)
		tk.Grid.rowconfigure(compFrame, i, weight=1)
		tk.Grid.columnconfigure(compFrame, i, weight=1)
		tk.Grid.rowconfigure(syrFrame, i, weight=1)
		tk.Grid.columnconfigure(syrFrame, i, weight=1)#
		tk.Grid.rowconfigure(txFrame, i, weight=1)
		tk.Grid.columnconfigure(txFrame, i, weight=1)
		tk.Grid.rowconfigure(logFrame, i, weight=1)
		tk.Grid.columnconfigure(logFrame, i, weight=1)
		tk.Grid.rowconfigure(commandsFrame, i, weight=1)
		tk.Grid.columnconfigure(commandsFrame, i, weight=1)
		tk.Grid.rowconfigure(jogFrame, i, weight=1)
		tk.Grid.columnconfigure(jogFrame, i, weight=1)
		tk.Grid.rowconfigure(jogBtnFrame, i, weight=1)
		tk.Grid.columnconfigure(jogBtnFrame, i, weight=1)
		tk.Grid.rowconfigure(rxFrame, i, weight=1)
		tk.Grid.columnconfigure(rxFrame, i, weight=1)
		tk.Grid.rowconfigure(serialFrame, i, weight=1)
		tk.Grid.columnconfigure(serialFrame, i, weight=1)
		tk.Grid.rowconfigure(statusbar, i, weight=1)
		tk.Grid.columnconfigure(statusbar, i, weight=1)


	jobLbl = tk.Label(jobFrame, text="Jobs").grid(row=0, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W)
	jobLstBox = Drag_and_Drop_Listbox(jobFrame, font=('Arial', 10)) #disabled
	jobLstBox.grid(row=1, column=0, columnspan=12, rowspan=11, sticky=tk.NSEW)
	jobScroll = tk.Scrollbar(jobFrame, orient=VERTICAL, command=jobLstBox.yview)
	jobScroll.grid(row=1, column=12, rowspan=11, sticky=(N, W, S))
	jobLstBox.configure(yscrollcommand=jobScroll.set)
	jobLstBox.bind('<Button-3>', showJobLstBoxMenu)
	jobLstBox.bind('<Control-X>', lambda x:cutJob())
	jobLstBox.bind('<Control-C>', lambda x:copyJob())
	jobLstBox.bind('<Control-V>', lambda x:pasteJob())
	jobLstBox.bind('<Insert>', lambda x:addJob())
	jobLstBox.bind('<Control-E>', lambda x:editJob())
	jobLstBox.bind('<Delete>', lambda x:deleteJob())
	jobLstBox.bind('<BackSpace>', lambda x:deleteJob())
	jobLstBox.bind('<Home>', lambda x:moveTopJob())
	jobLstBox.bind('<End>', lambda x:moveBottomJob())
	jobLstBox.bind('<Up>', lambda x:moveUpJob())
	jobLstBox.bind('<Down>', lambda x:moveDownJob())
	#Ctrl+A, Esc
	addBtn = tk.Button(jobFrame, text='Add...', width=9, command=addJob).grid(row=12, column=0, columnspan=3, padx=4, pady=4) # 
	editBtn = tk.Button(jobFrame, text='Edit', width=9, command=editJob).grid(row=12, column=3, columnspan=3, padx=4, pady=4)
	deleteBtn = tk.Button(jobFrame, text='Delete', width=9, command=deleteJob).grid(row=12, column=6, columnspan=3, padx=4, pady=4)
	clearBtn = tk.Button(jobFrame, text='Clear', width=9, command=clearJobs).grid(row=12, column=9, columnspan=3, padx=4, pady=4)
	runBtn = tk.Button(jobFrame, state=tk.DISABLED, image=runPic, width=20, height=20, command=clearOutputCmd)
	runBtn.grid(row=0, column=2, padx=4, pady=4)
	pauseBtn = tk.Button(jobFrame, state=tk.DISABLED, image=pausePic, width=20, height=20, command=setting)
	pauseBtn.grid(row=0, column=3, padx=4, pady=4)
	stopBtn = tk.Button(jobFrame, state=tk.DISABLED, image=stopPic, width=20, height=20, command=clearOutputCmd)
	stopBtn.grid(row=0, column=4, padx=4, pady=4)
	appendBtn = tk.Button(jobFrame, text='Append...', width=9, command=clearOutputCmd).grid(row=0, column=5, columnspan=2, padx=4, pady=4) #+insert?
	openBtn = tk.Button(jobFrame, text='Open...', width=9, command=setting).grid(row=0, column=7, columnspan=2, padx=4, pady=4)
	saveBtn = tk.Button(jobFrame, text='Save...', width=9, command=clearOutputCmd).grid(row=0, column=9, columnspan=2, padx=4, pady=4)
	movetopBtn = tk.Button(jobFrame, image=topPic, width=20, height=25, command=moveTopJob)
	movetopBtn.grid(row=4, column=13, padx=4, pady=4, sticky=tk.W)
	moveupBtn = tk.Button(jobFrame, image=upPic, width=20, height=25, command=moveUpJob)
	moveupBtn.grid(row=5, column=13, padx=4, pady=4, sticky=tk.W)
	movedownBtn = tk.Button(jobFrame, image=downPic, width=20, height=25, command=moveDownJob)
	movedownBtn.grid(row=6, column=13, padx=4, pady=4, sticky=tk.W)
	movebottomBtn = tk.Button(jobFrame, image=bottomPic, width=20, height=25, command=moveBottomJob)
	movebottomBtn.grid(row=7, column=13, padx=4, pady=4, sticky=tk.W) #draganddrop

	controlLbl = tk.Label(controlFrame, text="Controls").grid(row=0, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W)
	targetLbl = tk.Label(controlFrame, text="Target").grid(row=1, column=2, columnspan=2, padx=4, pady=4, sticky=tk.EW)
	currentLbl = tk.Label(controlFrame, text="Current").grid(row=1, column=4, columnspan=2, padx=4, pady=4)
	xLbl = tk.Label(controlFrame, width=2, text="X").grid(row=2, column=1, padx=2, pady=2, sticky=tk.S+tk.EW)
	yLbl = tk.Label(controlFrame, width=2, text="Y").grid(row=3, column=1, padx=2, pady=2, sticky=tk.S+tk.EW)
	zLbl = tk.Label(controlFrame, width=2, text="Z").grid(row=4, column=1, padx=2, pady=2, sticky=tk.S+tk.EW)
	aLbl = tk.Label(controlFrame, width=2, text="A").grid(row=5, column=1, padx=2, pady=2, sticky=tk.S+tk.EW)
	movexText = tk.Entry(controlFrame, width=8, validate='all', validatecommand=(moveValidation, '%P'))
	movexText.grid(row=2, column=2, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	moveyText = tk.Entry(controlFrame, width=8, validate='all', validatecommand=(moveValidation, '%P'))
	moveyText.grid(row=3, column=2, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	movezText = tk.Entry(controlFrame, width=8, validate='all', validatecommand=(moveValidation, '%P'))
	movezText.grid(row=4, column=2, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	moveaText = tk.Entry(controlFrame, width=8, validate='all', validatecommand=(moveValidation, '%P'))
	moveaText.grid(row=5, column=2, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	xCurrLbl = tk.Label(controlFrame, width=12, text="-")
	xCurrLbl.grid(row=2, column=4, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	yCurrLbl = tk.Label(controlFrame, width=12, text="-")
	yCurrLbl.grid(row=3, column=4, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	zCurrLbl = tk.Label(controlFrame, width=12, text="-")
	zCurrLbl.grid(row=4, column=4, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	aCurrLbl = tk.Label(controlFrame, width=12, text="-")
	aCurrLbl.grid(row=5, column=4, columnspan=2, padx=4, pady=4, sticky=tk.S+tk.EW)
	goBtn = tk.Button(controlFrame, width=12, text='Go', state=tk.DISABLED, command=GoCoord)
	goBtn.grid(row=6, column=2, columnspan=2, padx=4, pady=4, sticky=tk.SW)
	#getBtn = tk.Button(controlFrame, width=12, text='Get', state=tk.DISABLED, command=lambda:sendCmd(None))
	#getBtn.grid(row=6, column=5, columnspan=2, padx=15, pady=4, sticky=tk.SW)
	speedLbl = tk.Label(controlFrame, text="Speed:").grid(row=7, column=1, padx=4, pady=8, sticky=tk.S+tk.EW)
	speedSlider = tk.Scale(controlFrame, from_=1, to=100, variable=goSpeedVal, orient='horizontal', resolution=1, sliderlength=10) #, resolution=1, digits=0, sliderlength=50
	speedSlider.grid(row=7, column=2, columnspan=5, padx=2, pady=4, sticky=tk.SW)#min/max speed set!!!
	#speedValLbl = tk.Label(controlFrame, textvariable=goSpeedVal)
	#speedValLbl.grid(row=7, column=7, padx=4, pady=8, sticky=tk.S+tk.EW)
	absrelLbl = tk.Label(controlFrame, text="Target Type:").grid(row=8, column=1, columnspan=2, padx=4, ipadx=15, pady=8, sticky=tk.S+tk.EW)
	absRad = tk.Radiobutton(controlFrame, width=6, text="Abs", variable=goRelAbsVal, value=0)
	absRad.grid(row=8, column=3, columnspan=2, padx=2, pady=4, sticky=tk.S+tk.EW)#preferences
	relRad = tk.Radiobutton(controlFrame, width=6, text="Rel", variable=goRelAbsVal, value=1)
	relRad.grid(row=8, column=5, columnspan=2, padx=2, pady=4, sticky=tk.S+tk.EW)#preferences
	unitLbl = tk.Label(controlFrame, text="Unit Type:").grid(row=9, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	mmRad = tk.Radiobutton(controlFrame, width=6, text="mm", variable=coordUnitVal, value=0, command=clearEntry)
	mmRad.grid(row=9, column=3, columnspan=2, padx=2, pady=4, sticky=tk.S+tk.EW)#preferences
	stepRad = tk.Radiobutton(controlFrame, width=6, text="step", variable=coordUnitVal, value=1, command=clearEntry)
	stepRad.grid(row=9, column=5, columnspan=2, padx=2, pady=4, sticky=tk.S+tk.EW)#preferences

	settingsNtb.add(compFrame, text='Components')
	settingsNtb.add(syrFrame, text='Syringes')
	#compLbl = tk.Label(compFrame, text='Component').grid(row=1, column=0, padx=0, pady=12, sticky=tk.E)
	compCbo = ttk.Combobox(compFrame, width=20, state='readonly') #command/bind, (save, save all, restore, delete)
	compCbo.grid(row=0, column=0, columnspan=4, padx=12, pady=12, sticky=tk.W)
	compCbo['values'] = saved_components
	compCbo.set('Select Component')
	compNameLbl = tk.Label(compFrame, text="Name:").grid(row=1, column=0, padx=2, pady=2, sticky=tk.E)
	compNameText = tk.Entry(compFrame, width=12) #validate: no [], max length:
	compNameText.grid(row=1, column=1, columnspan=3, padx=4, pady=4, sticky=tk.W)
	compIdLbl = tk.Label(compFrame, text="ID:").grid(row=1, column=4, padx=2, pady=2, sticky=tk.E)
	compIdSpn = tk.Spinbox(compFrame, from_=1, to=12, textvariable=compIdVar, width=4) #id 0
	compIdSpn.grid(row=1, column=5, padx=4, pady=4, sticky=tk.W)
	compTypeLbl = tk.Label(compFrame, text="Type:").grid(row=2, column=0, padx=2, pady=2, sticky=tk.E)
	compTypeCbo = ttk.Combobox(compFrame, width=12, state='readonly')
	compTypeCbo.grid(row=2, column=1, columnspan=3, padx=2, pady=14, sticky=tk.W)
	compTypeCbo['values'] = COMPONENT_TYPES
	compTypeCbo.current(0)
	compInstCbt = tk.Checkbutton(compFrame, text='Installed', variable=compInstVar, onvalue=True, offvalue=False)
	compInstCbt.grid(row=2, column=4, columnspan=2, padx=2, pady=2)
	firstVialLbl = tk.Label(compFrame, width=2, text="First Vial").grid(row=3, column=0, columnspan=3, padx=5, pady=8, sticky=tk.S+tk.EW)
	lastVialLbl = tk.Label(compFrame, width=2, text="Last Vial").grid(row=3, column=3, columnspan=3, padx=5, pady=8, sticky=tk.S+tk.EW)
	dimLbl = tk.Label(compFrame, width=2, text="Dimensions").grid(row=6, column=0, columnspan=3, padx=5, pady=8, sticky=tk.S+tk.EW)
	zcoordLbl = tk.Label(compFrame, width=2, text="Z offset").grid(row=6, column=3, columnspan=3, padx=5, pady=8, sticky=tk.S+tk.EW)
	minxLbl = tk.Label(compFrame, width=2, text="X:").grid(row=4, column=0, padx=2, pady=4, sticky=tk.S+tk.E)
	minyLbl = tk.Label(compFrame, width=2, text="Y:").grid(row=5, column=0, padx=2, pady=4, sticky=tk.S+tk.E)
	maxxLbl = tk.Label(compFrame, width=2, text="X:").grid(row=4, column=3, padx=2, pady=4, sticky=tk.S+tk.E)
	maxyLbl = tk.Label(compFrame, width=2, text="Y:").grid(row=5, column=3, padx=2, pady=4, sticky=tk.S+tk.E)
	dimxLbl = tk.Label(compFrame, width=2, text="X:").grid(row=7, column=0, padx=2, pady=4, sticky=tk.S+tk.E)
	dimyLbl = tk.Label(compFrame, width=2, text="Y:").grid(row=8, column=0, padx=2, pady=4, sticky=tk.S+tk.E)
	minzLbl = tk.Label(compFrame, width=2, text="Z0:").grid(row=7, column=3, padx=2, pady=4, sticky=tk.S+tk.E)
	maxzLbl = tk.Label(compFrame, width=2, text="Zm:").grid(row=8, column=3, padx=2, pady=4, sticky=tk.S+tk.E)
	minxText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(validationCallback, '%P')) # check max x, y; y > 0
	minxText.grid(row=4, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	minyText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(validationCallback, '%P'))
	minyText.grid(row=5, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	maxxText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(validationCallback, '%P'))
	maxxText.grid(row=4, column=4, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	maxyText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(validationCallback, '%P'))
	maxyText.grid(row=5, column=4, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	dimxText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(uint8_Validation, '%P')) # int, >0
	dimxText.grid(row=7, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	dimyText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(uint8_Validation, '%P'))
	dimyText.grid(row=8, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	minzText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(uint8_Validation, '%P')) # max z, >0
	minzText.grid(row=7, column=4, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	maxzText = tk.Entry(compFrame, width=10, validate='all', validatecommand=(uint8_Validation, '%P'))
	maxzText.grid(row=8, column=4, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	compUploadBtn = tk.Button(compFrame, width=8, state=tk.DISABLED, text='Upload', command=clearOutputCmd)
	compUploadBtn.grid(row=9, column=1, columnspan=2, padx=14, pady=14, sticky=tk.E)
	compReadBtn = tk.Button(compFrame, width=8, state=tk.DISABLED, text='Read', command=clearOutputCmd)
	compReadBtn.grid(row=9, column=3, columnspan=2, padx=14, pady=14, sticky=tk.W)
	syrCbo = ttk.Combobox(syrFrame, width=20, state='readonly') #command/bind
	syrCbo.grid(row=0, column=0, columnspan=3, padx=12, pady=12, sticky=tk.S+tk.EW)
	syrCbo['values'] = saved_syringes
	syrCbo.set('Select Syringe')
	syrNameLbl = tk.Label(syrFrame, text="Name:").grid(row=1, column=0, padx=2, pady=2, sticky=tk.E)
	syrNameText = tk.Entry(syrFrame, width=8) #validate: no [], max length:
	syrNameText.grid(row=1, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	syrFlagLbl = tk.Label(syrFrame, text="Flags:").grid(row=2, column=0, padx=2, pady=2, sticky=tk.E)
	syrFlagText = tk.Entry(syrFrame, width=8, validate='all', validatecommand=(validationCallback, '%P')) # int, 0-255
	syrFlagText.grid(row=2, column=1, padx=4, pady=8, sticky=tk.S+tk.EW)
	syrInstCbt = tk.Checkbutton(syrFrame, text='Installed', variable=syrInstVar, onvalue=True, offvalue=False)
	syrInstCbt.grid(row=2, column=2, padx=2, pady=2, sticky=tk.E)
	maxVolLbl = tk.Label(syrFrame, width=2, text="Max Volume:").grid(row=3, column=0, padx=4, pady=4, sticky=tk.S+tk.EW)
	volPstepLbl = tk.Label(syrFrame, width=2, text="Vol/step:").grid(row=4, column=0, padx=4, pady=4, sticky=tk.S+tk.EW)
	overfillLbl = tk.Label(syrFrame, width=2, text="Overfill:").grid(row=5, column=0, padx=4, pady=4, sticky=tk.S+tk.EW)
	ndloffsetLbl = tk.Label(syrFrame, width=2, text="Ndl Offset:").grid(row=6, column=0, padx=4, pady=4, sticky=tk.S+tk.EW)
	maxVolText = tk.Entry(syrFrame, width=8, validate='all', validatecommand=(posFloatValidation, '%P')) # >0
	maxVolText.grid(row=3, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	volPstepText = tk.Entry(syrFrame, width=8, validate='all', validatecommand=(posFloatValidation, '%P')) # >0
	volPstepText.grid(row=4, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	overfillText = tk.Entry(syrFrame, width=8, validate='all', validatecommand=(validationCallback, '%P')) # >1
	overfillText.grid(row=5, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	ndloffsetText = tk.Entry(syrFrame, width=8, validate='all', validatecommand=(validationCallback, '%P')) # >=0
	ndloffsetText.grid(row=6, column=1, columnspan=2, padx=4, pady=8, sticky=tk.S+tk.EW)
	syrUploadBtn = tk.Button(syrFrame, width=8, state=tk.DISABLED, text='Upload', command=clearOutputCmd)
	syrUploadBtn.grid(row=8, column=0, padx=4, pady=14, sticky=tk.E)
	syrReadBtn = tk.Button(syrFrame, width=8, state=tk.DISABLED, text='Read', command=clearOutputCmd)
	syrReadBtn.grid(row=8, column=2, padx=4, pady=14, sticky=tk.W)


	commandsLbl = tk.Label(commandsFrame, text="Commands").grid(row=0, column=0, columnspan=3, padx=2, pady=2, sticky=tk.W)
	lcValveBtn = tk.Button(commandsFrame, width=12, state=tk.DISABLED, text='LC Valve Toggle', command=lambda:sendSerial('$MV', False)) #Inject/Waste
	lcValveBtn.grid(row=2, column=0, columnspan=3, padx=4, pady=4)
	fcValveBtn = tk.Button(commandsFrame, width=12, state=tk.DISABLED, text='FC Valve Toggle', command=lambda:sendSerial('$MC', False)) #Collect/Waste chr(39) chr(34)
	fcValveBtn.grid(row=3, column=0, columnspan=3, padx=4, pady=4)
	wash1Btn = tk.Button(commandsFrame, width=12, state=tk.DISABLED, text='Wash 1 Toggle', command=lambda:sendSerial(chr(160), False))
	wash1Btn.grid(row=4, column=0, columnspan=3, padx=4, pady=4)
	wash2Btn = tk.Button(commandsFrame, width=12, state=tk.DISABLED, text='Wash 2 Toggle', command=lambda:sendSerial(chr(161), False))
	wash2Btn.grid(row=5, column=0, columnspan=3, padx=4, pady=4)
	rmTipBtn = tk.Button(commandsFrame, width=10, state=tk.DISABLED, text='Remove Tip', command=lambda:sendSerial('$MR', False))
	rmTipBtn.grid(row=6, column=0, columnspan=3, padx=4, pady=4)
	goHomeBtn = tk.Button(commandsFrame, width=10, state=tk.DISABLED, text='Go Home', command=lambda:sendSerial('$MH', False))
	goHomeBtn.grid(row=7, column=0, columnspan=3, padx=4, pady=4)
	parkBtn = tk.Button(commandsFrame, width=10, state=tk.DISABLED, text='Park', command=lambda:sendSerial('$P', False))
	parkBtn.grid(row=8, column=0, columnspan=3, padx=4, pady=4)
	cycleStartBtn = tk.Button(commandsFrame, width=9, state=tk.DISABLED, text='Cycle Start', command=lambda:sendSerial('~', False))
	cycleStartBtn.grid(row=2, column=4, columnspan=3, padx=4, pady=4)
	feedHoldBtn = tk.Button(commandsFrame, width=9, state=tk.DISABLED, text='Feed Hold', command=lambda:sendSerial('!', False))
	feedHoldBtn.grid(row=3, column=4, columnspan=3, padx=4, pady=4)
	skipJobBtn = tk.Button(commandsFrame, width=9, state=tk.DISABLED, text='Skip Job', command=lambda:sendSerial('>', False))
	skipJobBtn.grid(row=4, column=4, columnspan=3, padx=4, pady=4)
	clearErrBtn = tk.Button(commandsFrame, width=9, state=tk.DISABLED, text='Clear Error', command=lambda:sendSerial('$X', False))
	clearErrBtn.grid(row=5, column=4, columnspan=3, padx=4, pady=4)
	resetBtn = tk.Button(commandsFrame, width=9, state=tk.DISABLED, text='Reset', command=lambda:sendSerial('@', False))
	resetBtn.grid(row=6, column=4, columnspan=3, padx=4, pady=4)
	#goTo...Btn = tk.Button(commandsFrame, width=12, text='Go To...', command=clearOutputCmd)
	# .grid(row=1, column=19, padx=4, pady=4)

	jogLbl = tk.Label(jogFrame, text="Jog").grid(row=0, column=0, columnspan=3, padx=4, pady=0, sticky=tk.W) #if state=busy, disable butons
	y_negBtn = tk.Button(jogBtnFrame, image=YdownPic, width=60, height=60, state=tk.DISABLED)
	y_negBtn.grid(row=0, column=5, columnspan=2, rowspan=2, padx=0, pady=0)
	x_negBtn = tk.Button(jogBtnFrame, image=XdownPic, width=60, height=60, state=tk.DISABLED)
	x_negBtn.grid(row=2, column=3, columnspan=2, rowspan=2, padx=0, pady=0)
	x_posBtn = tk.Button(jogBtnFrame, image=XupPic, width=60, height=60, state=tk.DISABLED)
	x_posBtn.grid(row=2, column=7, columnspan=2, rowspan=2, padx=0, pady=0)
	y_posBtn = tk.Button(jogBtnFrame, image=YupPic, width=60, height=60, state=tk.DISABLED)
	y_posBtn.grid(row=4, column=5, columnspan=2, rowspan=2, padx=0, pady=0)
	z_negBtn = tk.Button(jogBtnFrame, image=ZdownPic, width=60, height=60, state=tk.DISABLED)
	z_negBtn.grid(row=6, column=3, columnspan=2, rowspan=2, padx=0, pady=0)
	z_posBtn = tk.Button(jogBtnFrame, image=ZupPic, width=60, height=60, state=tk.DISABLED)
	z_posBtn.grid(row=8, column=3, columnspan=2, rowspan=2, padx=0, pady=0)
	a_posBtn = tk.Button(jogBtnFrame, image=AupPic, width=60, height=60, state=tk.DISABLED)
	a_posBtn.grid(row=6, column=7, columnspan=2, rowspan=2, padx=0, pady=0)
	a_negBtn = tk.Button(jogBtnFrame, image=AdownPic, width=60, height=60, state=tk.DISABLED)
	a_negBtn.grid(row=8, column=7, columnspan=2, rowspan=2, padx=0, pady=0)
	teachObjLbl = tk.Label(jogFrame, text="Teach Object:").grid(row=10, column=0, columnspan=2, padx=2, pady=2, sticky=tk.E)
	objSpn = tk.Spinbox(jogFrame, from_=1, to=12, textvariable=objNumVar, state=tk.DISABLED, width=4)
	objSpn.grid(row=10, column=2, padx=4, pady=4)
	teachZeroBtn = tk.Button(jogFrame, width=6, text='Start', state=tk.DISABLED, command=lambda:sendTeach('S'))
	teachZeroBtn.grid(row=10, column=4, columnspan=2, padx=4, pady=4)
	teachOffsetBtn = tk.Button(jogFrame, width=6, text='End', state=tk.DISABLED, command=lambda:sendTeach('E'))
	teachOffsetBtn.grid(row=10, column=6, columnspan=2, padx=4, pady=4)
	teachPenetrationBtn = tk.Button(jogFrame, width=6, text='Depth', state=tk.DISABLED, command=lambda:sendTeach('P'))
	teachPenetrationBtn.grid(row=10, column=8, columnspan=2, padx=4, pady=4)

	sentLbl = tk.Label(txFrame, text="Serial Tx").grid(row=0, column=0, columnspan=3, padx=2, pady=2, sticky=tk.SW)
	txText = tk.Entry(txFrame)
	txText.grid(row=1, column=0, columnspan=9, padx=4, pady=8, sticky=tk.S+tk.EW)
	txText.bind('<Up>', upKeyCmd)
	txText.bind('<Down>', downKeyCmd)
	txText.bind('<Button-3>', showTxTextMenu)
	sendBtn = tk.Button(txFrame, width=12, text='Send', state=tk.DISABLED, command=lambda:sendCmd(None))
	sendBtn.grid(row=1, column=9, columnspan=3, padx=4, pady=4, sticky=tk.SW)
	sentText = tkscroll.ScrolledText(txFrame, state=tk.DISABLED, font=('Courier', 10), wrap=tk.WORD)
	sentText.grid(row=2, column=0, columnspan=11, rowspan=11, padx=4, sticky=tk.NSEW)
	sentText.bind('<Button-3>', showRxTextMenu)

	rxLbl = tk.Label(rxFrame, text="Serial Rx").grid(row=0, column=0, columnspan=3, padx=2, pady=2, sticky=tk.SW)
	rxText = tkscroll.ScrolledText(rxFrame, state=tk.DISABLED, font=('Courier', 10), wrap=tk.WORD)
	rxText.grid(row=1, column=0, columnspan=11, rowspan=11, padx=4, sticky=tk.NSEW)
	rxText.bind('<Button-3>', showRxTextMenu)

	logLbl = tk.Label(logFrame, text="Logs").grid(row=0, column=0, padx=2, pady=2, sticky=tk.SW)
	logText = tkscroll.ScrolledText(logFrame, state=tk.DISABLED, font=('Courier', 10), wrap=tk.WORD)
	logText.grid(row=1, column=0, columnspan=11, rowspan=11, padx=4, sticky=tk.NSEW)
	logText.bind('<Button-3>', showRxTextMenu)

	connectBtn = tk.Button(serialFrame, width=12, text='Connect', command=connectPort)
	connectBtn.grid(row=0, column=1, columnspan=3, rowspan=2, padx=4, pady=4)
	settingBtn = tk.Button(serialFrame, width=12, text='Serial Settings', command=setting)
	settingBtn.grid(row=0, column=4, columnspan=3, rowspan=2, padx=4, pady=4)
	ASsettingBtn = tk.Button(serialFrame, width=12, text='AS Settings', command=as_setting)
	ASsettingBtn.grid(row=0, column=7, columnspan=3, rowspan=2, padx=4, pady=4)
	clearBtn = tk.Button(serialFrame, width=12, text='Clear Output', command=clearOutputCmd)
	clearBtn.grid(row=0, column=10, columnspan=3, rowspan=2, padx=4, pady=4)

	# jogStatusFrame = tk.Frame(statusbar)
	# jogStatusFrame.grid(column=0, row=0, sticky=tk.NSEW)
	jobStatusLbl = tk.Label(statusbar, text="Job Status:").grid(row=0, column=0, padx=2, pady=4, sticky=tk.E) #No current job;Running;Error#;Timeout
	jobStateLbl = tk.Label(statusbar, text="Not Connected")
	jobStateLbl.grid(row=0, column=1, padx=2, pady=4, sticky=tk.W)
	statusLbl = tk.Label(statusbar, text="State:").grid(row=0, column=4, padx=2, pady=4, sticky=tk.E)
	stateLbl = tk.Label(statusbar, text="Not Connected")
	stateLbl.grid(row=0, column=5, padx=2, pady=4, sticky=tk.W)
	asstatusLbl = tk.Label(statusbar, text="AS State:").grid(row=0, column=6, padx=2, pady=4, sticky=tk.E)
	asstateLbl = tk.Label(statusbar, text="Not Connected")
	asstateLbl.grid(row=0, column=7, padx=2, pady=4, sticky=tk.W)
	# LCvalveLbl = tk.Label(statusbar, text="LC valve:").grid(row=0, column=8, padx=2, pady=4, sticky=tk.E)
	# LCvalveStateLbl = tk.Label(statusbar, text="Not Connected")
	# LCvalveStateLbl.grid(row=0, column=9, padx=2, pady=4, sticky=tk.W)
	# FCvalveLbl = tk.Label(statusbar, text="FC valve:").grid(row=0, column=10, padx=2, pady=4, sticky=tk.E)
	# FCvalveStateLbl = tk.Label(statusbar, text="Not Connected")
	# FCvalveStateLbl.grid(row=0, column=11, padx=2, pady=4, sticky=tk.W)

	txTextMenu = tk.Menu(txText, tearoff=0)
	txTextMenu.add_command(label='Cut', accelerator='Ctrl+X', command=lambda:txText.event_generate('<<Cut>>'))
	txTextMenu.add_command(label='Copy', accelerator='Ctrl+C', command=lambda:txText.event_generate('<<Copy>>'))
	txTextMenu.add_command(label='Paste', accelerator='Ctrl+V', command=lambda:txText.event_generate('<<Paste>>'))

	rxTextMenu = tk.Menu(rxText, tearoff=0)
	rxTextMenu.add_command(label='Copy', accelerator='Ctrl+C', command=lambda:rxText.event_generate('<<Copy>>'))
	rxTextMenu.add_separator()
	rxTextMenu.add_checkbutton(label='Display in hexadecimal code', onvalue=True, offvalue=False, variable=dispHexVar)

	jobLstBoxMenu = tk.Menu(jobLstBox, tearoff=0) #shortcuts
	jobLstBoxMenu.add_command(label='Cut', accelerator='Ctrl+X', command=cutJob)
	jobLstBoxMenu.add_command(label='Copy', accelerator='Ctrl+C', command=copyJob)
	jobLstBoxMenu.add_command(label='Paste', accelerator='Ctrl+V', command=pasteJob)
	jobLstBoxMenu.add_separator()
	jobLstBoxMenu.add_command(label='Insert...', accelerator='Ctrl+A', command=addJob)
	jobLstBoxMenu.add_command(label='Edit...', accelerator='Ctrl+E', command=editJob)
	jobLstBoxMenu.add_command(label='Delete', accelerator='Del', command=deleteJob)
	jobLstBoxMenu.add_separator()
	jobLstBoxMenu.add_command(label='Move Top', accelerator='Home', command=moveTopJob)
	jobLstBoxMenu.add_command(label='Move Bottom', accelerator='End', command=moveBottomJob)

	listPortsPolling()

	root.update()
	sw = root.winfo_screenwidth()
	sh = root.winfo_screenheight()
	rw = root.winfo_width()
	rh = root.winfo_height()
	root.minsize(1280, 960)
	#root.geometry(f'{rw}x{rh}+{int((sw-rw)/2)}+{int((sh-rh)/2)-30}')
	#wtx =\
	#writeConsoles(str(sw) + ';' + str(sh) + ';' + str(rw) + ';' + str(rh), 2)

	root.state('zoomed')
	root.mainloop()