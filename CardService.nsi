################################################################################
# step.0 : Include 'Modern UI' & '.nsh'
!include MUI2.nsh
!include UpgradeDLL.nsh
!include WordFunc.nsh
!include InstallOptions.nsh
!include "UninstallHelper.nsh"
!include x64.nsh
!include "FileFunc.nsh"


################################################################################
# step.1 : Define environment
	!define CLASS_NAME 		"SCardManager"		; ������
	!define TITLE_NAME		"���� ���� ��� ����"	; ������
	!define TITLE_NAME_INST	"���� ���� ��� ����"	; ������

	!define HOMEPAGE_URL	"http://card.snu.ac.kr/"
	
	!define INSTALLER_NAME			"SCardSvcInst.exe"
	!define UNINSTALLER_NAME		"SCardSvcUninstall.exe"
	!define DEFAULT_INSTDIR			"$PROGRAMFILES\SCardManager"
	!define EXCUTE_NAME				"SCardManager.exe"
	!define SERVICE_NAME			"SCardService.exe"

	!define REG_ROOT					"Software\Symtra\SCardManager"	; ������
	!define REG_KEYNAME_INSTDIR			"InstallPath"	; ������

	!define	 MAKE_UNINSTALLER	; ���ν��緯�� �����ؾ��� ��� �����ؾ���

var /GLOBAL EXECUTE_PARAMETER
var /GLOBAL INSTALL_MODE             ;���Ϸ�Ʈ���, �Ϲݼ�ġ ���

################################################################################
# step.2 : General setting
	# style
	RequestExecutionLevel admin
	XPStyle on
	CRCCheck on
	WindowIcon on
	AutoCloseWindow	true
	DirText "" "" "ã�ƺ���" ""
	BrandingText "${TITLE_NAME}"
	UninstallButtonText "����"
	
	# Name and file
	Name		"${TITLE_NAME}"
	OutFile		"output\${INSTALLER_NAME}"
	InstallDir	"${DEFAULT_INSTDIR}"

################################################################################
# step.3 : Interface configuration
	# Page header
	!define MUI_ICON 	"SCardManager.ico"
	!define MUI_UNICON 	"waste.ico"

	!define MUI_HEADERIMAGE
		!define MUI_HEADERIMAGE_RIGHT
		!define MUI_HEADERIMAGE_BITMAP		"CardSvcInst2.bmp"
			!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH
		!define MUI_HEADERIMAGE_UNBITMAP	"CardSvcInst2.bmp"
			!define MUI_HEADERIMAGE_UNBITMAP_NOSTRETCH

	# Installer welcome/finish page
	!define MUI_WELCOMEFINISHPAGE_BITMAP	"CardSvcInst.bmp"

	# Uninstaller welcome/finish page
	!define MUI_UNWELCOMEFINISHPAGE_BITMAP	"CardSvcInst.bmp"

	# License page

	# Components page

	# Directory page

	# Start Menu folder page

	# Installation page

	# Installer finish page
	!define MUI_FINISHPAGE_NOAUTOCLOSE

	# Uninstaller finish page
	!define MUI_UNFINISHPAGE_NOAUTOCLOSE

	# Abort warning
	!define MUI_ABORTWARNING

	# Uninstaller abort warning
	!define MUI_UNABORTWARNING

	# Language setting
	!define MUI_EULRUL	"��"	; Do must copy custom 'korean.nsh & korean.nlf' files to 'NSIS' language folder.



################################################################################
# step.4 : Pages
	# Installer pages
		# MUI_PAGE_WELCOME
		# MUI_PAGE_COMPONENTS
		# MUI_PAGE_DIRECTORY
		# MUI_PAGE_STARTMENU pageid variable
		# MUI_PAGE_INSTFILES
		# MUI_PAGE_FINISH

	# General page settings
	#!define MUI_CUSTOMFUNCTION_GUIINIT GuiInit
	
	# Welcome page settings
	!define MUI_WELCOMEPAGE_TITLE	"${TITLE_NAME}�� ��ġ�մϴ�."

	!insertmacro MUI_PAGE_WELCOME
	
	
	# Components page settings
	
	# Directory page settings
	!define MUI_DIRECTORYPAGE_TEXT_TOP	"${TITLE_NAME}${MUI_EULRUL} ���� ������ ��ġ�� �����Դϴ�.(����)$\r$\n\
										�ٸ� ������ ��ġ�ϰ� �����ø� $\"$(^BrowseBtn)$\" ��ư�� ������ �ٸ� ������ ������ �ּ���.$\r$\n\
										��ġ�� �����Ϸ��� $\"$(^InstallBtn)$\" ��ư�� ���� �ּ���."

	!define MUI_DIRECTORYPAGE_VERIFYONLEAVE

	!insertmacro MUI_PAGE_DIRECTORY
	
	# Start Menu folder page settings
	
	# Installation page settings
	!insertmacro MUI_PAGE_INSTFILES
	
	# Finish page settings
	!define MUI_FINISHPAGE_TITLE	"${TITLE_NAME}�� ��ġ�Ǿ����ϴ�."
	!define MUI_FINISHPAGE_TEXT		"${TITLE_NAME}�� ��ġ �Ϸ�Ǿ����ϴ�."

	#!define MUI_FINISHPAGE_RUN

	!define MUI_PAGE_CUSTOMFUNCTION_SHOW	FinishPageShow
	!define MUI_PAGE_CUSTOMFUNCTION_LEAVE	FinishPageLeave
	!insertmacro MUI_PAGE_FINISH


	# Uninstaller pages
		# MUI_UNPAGE_WELCOME
		# MUI_UNPAGE_CONFIRM
		# MUI_UNPAGE_LICENSE textfile
		# MUI_UNPAGE_COMPONENTS
		# MUI_UNPAGE_DIRECTORY
		# MUI_UNPAGE_INSTFILES
		# MUI_UNPAGE_FINISH

!ifdef MAKE_UNINSTALLER
	!echo "Do write Uninstaller page script"
	
	# Welcome page settings
	!define MUI_WELCOMEPAGE_TEXT "${TITLE_NAME} ���Ÿ� ���Ͻø� $\"$(^UninstallBtn)$\" ��ư�� �����ּ���."
	!insertmacro MUI_UNPAGE_WELCOME
	
	# UnInstallation page settings
	!insertmacro MUI_UNPAGE_INSTFILES

	# Finish page settings
	!define MUI_FINISHPAGE_TITLE	"${TITLE_NAME} ���Ű� �Ϸ� �Ǿ����ϴ�."
	!define MUI_FINISHPAGE_TEXT		"${TITLE_NAME} ���α׷��� ���ŵǾ����ϴ�."
	!define MUI_PAGE_CUSTOMFUNCTION_SHOW	un.FinishPageShow
	!insertmacro MUI_UNPAGE_FINISH
	
	# Custom page settings (for survey page)
	#!define MUI_FINISHPAGE_TITLE	"${TITLE_NAME}${MUI_EULRUL} �̿��� �ּż� �����մϴ�."
	#!define MUI_PAGE_CUSTOMFUNCTION_SHOW	un.SurveyPageShow
	#!define MUI_PAGE_CUSTOMFUNCTION_LEAVE	un.SurveyPageLeave
	#!insertmacro MUI_UNPAGE_FINISH
!else
	!echo "Skip Uninstaller page script"
!endif
	
################################################################################
# step.5 : Language files
	!insertmacro MUI_LANGUAGE "Korean"
	!insertmacro MUI_LANGUAGE "English"
	;!insertmacro MUI_RESERVEFILE_LANGDLL ;Language selection dialog;


################################################################################
# step.6 : Reserve files

################################################################################
# step.6 : Set App Info

	!searchparse /ignorecase /file './SCardManager/SCardManager.rc' 'ProductVersion' 'VERSION_NAME' ''
   	!searchreplace /ignorecase VERSION_NAME ${VERSION_NAME} "," "."

	VIAddVersionKey /LANG=1042-Korean "ProductVersion" "${VERSION_NAME}"
	VIAddVersionKey /LANG=1042-Korean "ProductName" "SCardManager"
	VIAddVersionKey /LANG=1042-Korean "CompanyName" "(c) Symtra"
	VIAddVersionKey /LANG=1042-Korean "FileVersion" "${VERSION_NAME}"
	VIAddVersionKey /LANG=1042-Korean "InternalName" "SCard Manager"
	VIAddVersionKey /LANG=1042-Korean "FileDescription" "SCard Manager"
	VIAddVersionKey /LANG=1042-Korean "LegalCopyright" "(c) Symtra. All rights reserved."
	VIProductVersion "${VERSION_NAME}"


################################################################################
# Sections
################################################################################

Section -openlogfile

   # uninstall string�� ���ν��緯 path�� ��ϵǾ� ������ ��ġ�� ������ ����. 
   # uninstall string�� ���� ��ġ�� path�� ���ν��緯 path�� �ٸ��� ��ġ ���
	ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "UninstallString" 
	ReadRegStr $1 HKLM "${REG_ROOT}" "${REG_KEYNAME_INSTDIR}"
	StrLen $2 $0
	${If} $2 == 0
	  Goto CrtInstDir
	${EndIf}	
    StrCmp $0 "$1\${UNINSTALLER_NAME}" 0 CrtInstDir

true:  
		ExecShell "open" "$0\${UNINSTALLER_NAME}"
false:
		 Quit

CrtInstDir:

	CreateDirectory "$INSTDIR"
	IfFileExists "$INSTDIR\${UninstLog}" +3
		FileOpen $UninstLog "$INSTDIR\${UninstLog}" w
	Goto +4
		SetFileAttributes "$INSTDIR\${UninstLog}" NORMAL
		FileOpen $UninstLog "$INSTDIR\${UninstLog}" a
		FileSeek $UninstLog 0 END
SectionEnd


Section InstRequirement InstRequirement
	SetShellVarContext all
	SetOverwrite try
	
!ifdef MAKE_UNINSTALLER
	!echo "Create uninstaller.exe"
	# Create uninstaller
	${WriteUninstaller} "$INSTDIR\${UNINSTALLER_NAME}"
!else
	!echo "Do not create uninstaller.exe"
!endif

	# Copy Files
	${SetOutPath} "$INSTDIR"
	
	ClearErrors
chkSVCFile:

	${File} ".\Output\X86\" "SCardService.exe"
	${File} ".\Output\X86\" "SCardManager.exe"
	${File} ".\Output\X86\" "SCardMgrLauncher.exe"
	${File} ".\Output\X86\" "AppPList.ini"

	SetOutPath "$INSTDIR"	; set excute folder
	
	# ShortCut
	CreateDirectory "$SMPROGRAMS\${TITLE_NAME_INST}"
		CreateShortCut "$SMPROGRAMS\${TITLE_NAME_INST}\${TITLE_NAME}.lnk" "$INSTDIR\${EXCUTE_NAME}" "" "" 0 SW_SHOWNORMAL "" "${TITLE_NAME} ����"
		CreateShortCut "$SMPROGRAMS\${TITLE_NAME_INST}\${TITLE_NAME} ����.lnk" "$INSTDIR\${UNINSTALLER_NAME}" "" "" 0 SW_SHOWNORMAL "" "${TITLE_NAME} ����"
	CreateShortCut "$DESKTOP\${TITLE_NAME}.lnk" "$INSTDIR\${EXCUTE_NAME}" "" "" 0 SW_SHOWNORMAL "" "${TITLE_NAME} ����"
	CreateShortCut "$STARTMENU\${TITLE_NAME}.lnk" "$INSTDIR\${EXCUTE_NAME}" "" "" 0 SW_SHOWNORMAL "" "${TITLE_NAME} ����"
	

	# Register dll


	ExecWait '"$INSTDIR\{SERVICE_NAME}" /service'


	
	# Registry setting
	WriteRegStr	HKLM "${REG_ROOT}" "${REG_KEYNAME_INSTDIR}" "$INSTDIR"
	
	${GetFileVersion} "$INSTDIR\${APP_EXENAME}" $R0	
	
	# Control Panel
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "DisplayName" "${TITLE_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "UninstallString" "$INSTDIR\${UNINSTALLER_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "DisplayIcon" "$INSTDIR\${EXCUTE_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "DisplayVersion" $R0
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager" "Publisher" "Symtra"	
SectionEnd


Section -closelogfile
	FileClose $UninstLog
	SetFileAttributes "$INSTDIR\${UninstLog}" READONLY|SYSTEM|HIDDEN
SectionEnd


 Function un.GetParameters

   Push $R0
   Push $R1
   Push $R2
   Push $R3

   StrCpy $R2 1
   StrLen $R3 $CMDLINE

   ;Check for quote or space
   StrCpy $R0 $CMDLINE $R2
   StrCmp $R0 '"' 0 +3
     StrCpy $R1 '"'
     Goto loop
   StrCpy $R1 " "

   loop:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 $R1 get
     StrCmp $R2 $R3 get
     Goto loop

   get:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " get
     StrCpy $R0 $CMDLINE "" $R2

   Pop $R3
   Pop $R2
   Pop $R1
   Exch $R0

 FunctionEnd 

 !ifdef MAKE_UNINSTALLER
	!echo "Write uninstaller section"


Section Uninstall
	SetShellVarContext all
	SetAutoClose true

  Call un.GetParameters
  Pop $EXECUTE_PARAMETER

  ${GetOptions} $EXECUTE_PARAMETER "/m" $R0
  IfErrors ContinueUninst


ContinueUninst:
	# Can't uninstall if uninstall log is missing!
	IfFileExists "$INSTDIR\${UninstLog}" +6
	MessageBox MB_OK|MB_ICONSTOP "${UninstLog} ������ ã�� �� �����ϴ�!$\r$\n���������� ������ �� �����ϴ�!"
	abort


	
	lbl_continue_uninstall:
	
	# UnRegister ShellExtension


	ExecWait '"$INSTDIR\{SERVICE_NAME}" /unregserver'	
	
	
	# Delete files
	Push $R0
	Push $R1
	Push $R2
	SetFileAttributes "$INSTDIR\${UninstLog}" NORMAL
	FileOpen $UninstLog "$INSTDIR\${UninstLog}" r
	StrCpy $R1 0
 
	GetLineCount:
		ClearErrors
			FileRead $UninstLog $R0
			IntOp $R1 $R1 + 1
			IfErrors 0 GetLineCount
 
	LoopRead:
		FileSeek $UninstLog 0 SET
		StrCpy $R2 0
		FindLine:
			FileRead $UninstLog $R0
			IntOp $R2 $R2 + 1
			StrCmp $R1 $R2 0 FindLine
 
		StrCpy $R0 $R0 -2
		IfFileExists "$R0\*.*" 0 +3
			RMDir /REBOOTOK $R0  #is dir
		Goto +3
		IfFileExists $R0 0 +2
			Delete /REBOOTOK $R0 #is file
 
		IntOp $R1 $R1 - 1
		StrCmp $R1 0 LoopDone
		Goto LoopRead

	LoopDone:
	
	FileClose $UninstLog
	Delete /REBOOTOK "$INSTDIR\${UninstLog}"
	Pop $R2
	Pop $R1
	Pop $R0

	RMDir /REBOOTOK "$INSTDIR"
	;RMDir /r /REBOOTOK "$INSTDIR"	; This is very dangerous
	RMDir /r "$SMPROGRAMS\${TITLE_NAME_INST}"
	Delete /REBOOTOK "$DESKTOP\${TITLE_NAME}.lnk"
	Delete /REBOOTOK "$STARTMENU\${TITLE_NAME}.lnk"
	SetRebootFlag false
		
	# Registry

	StrCpy $0 0
	loop:
		EnumRegKey $1 HKU "" $0
		StrCmp $1 "" done
		IntOp $0 $0 + 1
		DeleteRegKey HKU "$1\${REG_ROOT}"
		Goto loop 
	done:

	# Control Panel
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SCardManager"
	DeleteRegKey HKLM "${REG_ROOT}"
	
	DeleteRegKey HKU "Software\Symtra"		
SectionEnd
!else
	!echo "Do not write uninstaller section"
!endif


################################################################################
# ����� ���� Functions
################################################################################

################################################################################

################################################################################
# �ν��� �������� ȣ��Ǵ� �Լ�
Function GetParameters

   Push $R0
   Push $R1
   Push $R2
   Push $R3

   StrCpy $R2 1
   StrLen $R3 $CMDLINE

   ;Check for quote or space
   StrCpy $R0 $CMDLINE $R2
   StrCmp $R0 '"' 0 +3
     StrCpy $R1 '"'
     Goto loop
   StrCpy $R1 " "

   loop:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 $R1 get
     StrCmp $R2 $R3 get
     Goto loop

   get:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " get
     StrCpy $R0 $CMDLINE "" $R2

   Pop $R3
   Pop $R2
   Pop $R1
   Exch $R0

 FunctionEnd 

Function .onInit
	System::Call 'kernel32::CreateMutexA(i 0, i 0, t "InstMutex") i .r1 ?e'
	Pop $R0
	StrCmp $R0 0 +6
	IfSilent 0 +2		
		WriteRegStr HKLM REG_ROOT "InstallErrorMsg" "��ġ �Ǵ� ���Ÿ� ���� ���̾ ������ �� �����ϴ�."
		MessageBox MB_OK|MB_ICONSTOP "��ġ �Ǵ� ���Ÿ� ���� ���̾ ������ �� �����ϴ�."
		Abort

   ClearErrors
   
# D �ɼ� ���� Ȯ�� 
  Call GetParameters
  Pop $EXECUTE_PARAMETER

  ${GetOptions} $EXECUTE_PARAMETER "/D" $R1
  ${If} $R1 != ""
    StrCpy $INSTDIR $R1
  ${EndIf}

  ${GetOptions} $EXECUTE_PARAMETER "/m" $R0

		
	lbl_ContinueOS:
	

	lbl_InstallStart:
	
	# install size == 150%
	SectionGetSize ${InstRequirement} $0
		IntOp $1 $0 / 2
		IntOp $0 $0 + $1
	SectionSetSize ${InstRequirement} $0		
		

FunctionEnd


################################################################################
# �ν����� ���������� �Ϸ�ǰ� ȣ��Ǵ� �Լ�
Function .onInstSuccess
FunctionEnd


################################################################################



################################################################################
# �ν��� �Ϸ� �������� ���϶� ȣ���
Function FinishPageShow	
	
FunctionEnd


################################################################################
# �ν��� �Ϸ� �������� ����� ȣ���
Function FinishPageLeave

FunctionEnd


################################################################################
# ���ν����� ���۵Ǳ����� ȣ��Ǵ� �Լ�
Function un.onInit
	System::Call 'kernel32::CreateMutexA(i 0, i 0, t "SCardManagerInstMutex") i .r1 ?e'
	Pop $R0
	StrCmp $R0 0 +3
		MessageBox MB_OK|MB_ICONSTOP "��ġ �Ǵ� ���Ÿ� ���� ���̾ ������ �� �����ϴ�."
		Abort
	lbl_UninstallStart:
FunctionEnd


################################################################################
# ���ν����� �Ϸ�ǰ� ȣ��Ǵ� �Լ�
Function un.onUninstSuccess
FunctionEnd


################################################################################
# ���ν��� Finish �������� �������鼭 ȣ��Ǵ� �Լ�
Function un.FinishPageShow
	GetDlgItem $0 $HWNDPARENT 2
	EnableWindow $0 0
FunctionEnd

