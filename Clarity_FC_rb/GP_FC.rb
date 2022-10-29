
include UNI

#########################################################################
# Sub-device class expected by framework.
#
# Sub-device represents functional part of the chromatography hardware.
# FRC implementation.
#########################################################################
# Timer period [ms]
$timerPeriod = 400
class FRC < FRCSubDeviceWrapper
	# Constructor. Call base and do nothing. Make your initialization in the Init function instead.
	def initialize
		super
	end
	#########################################################################
	# Method expected by framework.
	#
	# Initialize FRC sub-device. 
	# Set sub-device name, specify method items, specify monitor items, ...
	# Returns nothing.
	#########################################################################	
	def Init
		#Trace(">>> FRC.Init\n")
	end
end # class FRC

#########################################################################
# Device class expected by framework.
#
# Basic class for access to the chromatography hardware.
# Maintains a set of sub-devices.
# Device represents whole box while sub-device represents particular 
# functional part of chromatography hardware.
# The class name has to be set to "Device" because the device instance
# is created from the C++ code and the "Device" name is expected.
#########################################################################
class Device < DeviceWrapper
	# Constructor. Call base and do nothing. Make your initialization in the Init function instead.
	def initialize
		super
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Initialize configuration data object of the device and nothing else
	# (set device name, add all sub-devices, setup configuration, set pipe
	# configurations for communication, #  ...).
	# Returns nothing.
	#########################################################################	
	def InitConfiguration
		Configuration().AddString("FRCName", t("NAME"), "FC 1", "VerifyFRCName")
		Configuration().AddInt("ValvePositions", t("MAX_POSITIONS"), 100, "VerifyPositions")		
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Initialize device. Configuration object is already initialized and filled with previously stored values.
	# (set device name, add all sub-devices, setup configuration, set pipe
	# configurations for communication, #  ...).
	# Returns nothing.
	#########################################################################	
	def Init
		@m_FRC=FRC.new
		AddSubDevice(@m_FRC)
		@m_FRC.SetName(Configuration().GetString("FRCName"))
		SetName("FC GP")
		SetDefaultFRCStartVial(1)
		SetDefaultFRCEndVial(Configuration().GetInt("ValvePositions"))
		SetTimerPeriod($timerPeriod)
 	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Sets communication parameters.
	# Returns nothing.
	#########################################################################	
	def InitCommunication()
	end
	
	#########################################################################
	# User written method.
	#
	# Validates length of FRC name.
	# Validation function returns true when validation is successful otherwise
	# it returns message which will be shown in the Message box.	
	#########################################################################
	def VerifyFRCName(uiitemcollection,value)
		if (value.length >= 32)
			return t("NAME_LONG")
		end
		return true
	end
	
	#########################################################################
	# User written method.
	#
	# Validates number of positions
	# Validation function returns true when validation is successful otherwise
	# it returns message which will be shown in the Message box.	
	#########################################################################
	def VerifyPositions(uiitemcollection,value)
		if (value < 1 or value > 10000)
			return t("CHECK_POSITIONS")
		end
		return true
	end
	
	#########################################################################
	# Method expected by framework
	#
	# Here you should check leading and ending sequence of characters, 
	# check sum, etc. If any error occurred, use ReportError function.
	#	dataArraySent - sent buffer (can be nil, so it has to be checked 
	#						before use if it isn't nil), array of bytes 
	#						(values are in the range <0, 255>).
	#	dataArrayReceived - received buffer, array of bytes 
	#						(values are in the range <0, 255>).
	# Returns true if frame is found otherwise false.		
	#########################################################################	
	def FindFrame(dataArraySent, dataArrayReceived)
		return false
	end
	
	#########################################################################
	# Method expected by framework
	#
	# Return true if received frame (dataArrayReceived) is answer to command
	# sent previously in dataArraySent.
	#	dataArraySent - sent buffer, array of bytes 
	#						(values are in the range <0, 255>).
	#	dataArrayReceived - received buffer, array of bytes 
	#						(values are in the range <0, 255>).
	# Return true if in the received buffer is answer to the command 
	# from the sent buffer. 
	#########################################################################		
	def IsItAnswer(dataArraySent, dataArrayReceived)
		return true
	end
	
	#########################################################################
	# Method expected by framework
	#
	# Returns serial number string from HW (to comply with CFR21) when 
	# succeessful otherwise false or nil. If not supported return false or nil.
	#########################################################################	
	def CmdGetSN
		# Serial number not supported in the hw.
		return false
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when instrument opens
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdOpenInstrument
		#Trace(">>> CmdOpenInstrument\n")
		# Nothing to send.
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when sequence starts
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdStartSequence
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when sequence resumes
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdResumeSequence
		# Nothing to send.
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when run starts
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdStartRun
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when injection performed
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdPerformInjection
		# Nothing to send.
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when injection bypassed
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdByPassInjection
		# Nothing to send.
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Starts method in HW.
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdStartAcquisition
		Monitor().SetRunning(true)
		Monitor().Synchronize()
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when acquisition restarts
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdRestartAcquisition
		# Nothing to send.
		return true
	end

	#########################################################################
	# Method expected by framework.
	#
	# Stops running method in hardware. 
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdStopAcquisition
	 	Monitor().SetRunning(false)
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Aborts running method or current operation. Sets initial state.
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdAbortRunError
		return CmdStopAcquisition()
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Aborts running method or current operation (request from user). Sets initial state.
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdAbortRunUser
		return CmdStopAcquisition()
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Aborts running method or current operation (shutdown). Sets initial state.
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdShutDown
		CmdAbortRunError()	
		CmdFRCWaste()
		Monitor().Synchronize()
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when run stops
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdStopRun
		# Nothing to send.
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when sequence stops
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdStopSequence
		# Nothing to send.
		CmdFRCWaste()
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# gets called when closing instrument
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdCloseInstrument
		# Nothing to send.
		return true
	end

	#########################################################################
	# Method expected by framework.
	#
	# Tests whether hardware device is present on the other end of the communication line.
	# Send some simple command with fast response and check, whether it has made it
	# through pipe and back successfully.
	# Returns true when successful otherwise false.
	#########################################################################
	def CmdTestConnect
		# Reading of valve position
		return true
	end

	#########################################################################
	# Method expected by framework.
	#
	# Send method to hardware.
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdSendMethod
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Loads method from hardware.
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdLoadMethod(method)
		return true		
	end
		
	#########################################################################
	# Method expected by framework.
	#
	# Duration of thermostat method.
	# Returns complete (from start of acquisition) length (in minutes) 
	# 	of the current method in sub-device (can use GetRunLengthTime()).
	# Returns METHOD_FINISHED when hardware instrument is not to be waited for or 
	# 	method is not implemented.
	# Returns METHOD_IN_PROCESS when hardware instrument currently processes 
	# 	the method and sub-device cannot tell how long it will take.
	#########################################################################
	def GetMethodLength
		return METHOD_FINISHED
	end	
	
	#########################################################################
	# Method expected by framework.
	#
	# Periodically called function which should update state 
	# of the sub-device and monitor.
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdTimer
	    return true
	end
	
	#########################################################################
	# Method expected by framework
	#
	# gets called when user presses autodetect button in configuration dialog box
	# return true or  false
	#########################################################################
	def CmdAutoDetect
		return true
	end
	
	#########################################################################
	# Method expected by framework
	#
	# Processes unrequested data sent by hardware. Unrequested data is not 
	# supported for now please use default implementation from examples.
	#	dataArrayReceived - received buffer, array of bytes 
	#						(values are in the range <0, 255>).
	# Returns true if frame was processed otherwise false.
	#########################################################################
	def ParseReceivedFrame(dataArrayReceived)
		# Passes received frame to appropriate sub-device's ParseReceivedFrame function.
	end	

	#########################################################################
	# Method expected by framework.
	#
	# Start Collect
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdFRCCollect()
	Trace("collect\n")
	  	return true		
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Start Wasting
	# Returns true when successful otherwise false.	
	#########################################################################
	def CmdFRCWaste()
	Trace("waste\n")
		return true
	end

	#########################################################################
	# Method written by user.
	#
	# If valve configured as with waste position and currently wasting, then just remembers requested positio
	# otherwise sets requested position immediately
	#########################################################################
	def CmdSetFRCPosition(nPosition)
		return true
	end
	
	#########################################################################
	# Method expected by framework.
	#
	# Send request to go to next valve position. See CmdSetFRCPosition
	#########################################################################
	def CmdNextFRCPosition
	Trace("next\n")
		return true
	end
	
	#########################################################################
	# Required by Framework
	#
	# Gets called to obtail real fial number of current fraction
	#########################################################################
	def CmdGetFRCPosition
		return true
	end
	
	#########################################################################
	# Required by Framework
	#
	# Gets called when chromatogram is acquired, chromatogram might not exist at the time.
	#########################################################################
	def NotifyChromatogramFileName(chromatogramFileName)
	end
	
	#########################################################################
	# Required by Framework
	#
	# Gets called when chromatogram is acquired, chromatogram might not exist at the time.
	#########################################################################
	def CheckMethod(situation,method)
		if (situation == Chm_MethodDialog)
		   Trace(method.GetFRCEndVial().to_s)
		end
		return true
	end

	#########################################################################
	# User written method.
	#
	# Returns translated string with specified ID
	#########################################################################
	def t(stringID)  
		$locale = GetLanguageCode()       # ask Clarity about current language code
		defaultLang = "ENG"
		#TraceLine ("Lang:" << $locale.to_s)
	   
		if (!$locale)  
			$locale = defaultLang
		elsif (!$translation.has_key?($locale))
			#Missing language - set language to default
			$locale = defaultLang
		end
	   
		if ($translation.has_key?($locale) && $translation[$locale].has_key?(stringID))
			return $translation[$locale][stringID]
		elsif ($locale != defaultLang && $translation.has_key?(defaultLang) && $translation[defaultLang].has_key?(stringID))
			#Chosen language does not have translation with given ID, use default language instead
			return $translation[defaultLang][stringID]   
		end  
	   
		return stringID + " string not found"
	 end

end # class Device

$translation = {
  "ENG" => {
	"NAME" => "FC Name",	
	"NAME_LONG" => "Name too long.",
	"CHECK_POSITIONS" => "Invalid Maximum Number of Positions",
	"MAX_POSITIONS" => "Maximum Number of Positions"
		},
	"CHS" => {
		#Generated by rcupdate Fri Apr 19 14:56:22 2019
		"NAME" => "FC名称",
		"CHECK_POSITIONS" => "无效的最大位置数",
		"NAME_LONG" => "名称太长。"
		},
	"DEU" => {
		#Generated by rcupdate Fri Apr 19 14:57:09 2019
		"NAME" => "FC Name",
		"CHECK_POSITIONS" => "Ungültige Maximalanzahl der Positionen",
		"NAME_LONG" => "Name zu lang."
		},
	"FRA" => {
		#Generated by rcupdate Fri Apr 19 14:59:21 2019
		"NAME" => "Nom FC",
		"CHECK_POSITIONS" => "Nombre Maximum de Positions Non Valide",
		"NAME_LONG" => "Nom trop long."
		}
}
