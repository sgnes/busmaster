/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Implementation file for CTxMsgDetailsView class
 * @author Raja N
 * @copyright Copyright (c) 2011, Robert Bosch Engineering and Business Solutions. All rights reserved.
 *
 * Implementation file for CTxMsgDetailsView class
 */

#include "TxWindow_stdafx.h"             // For standard includes
#include "include/BaseDefs.h"
#include "include/struct_can.h"
#include "DataTypes/MsgSignal_Datatypes.h"
#include "Utility/ComboItem.h"          // For Custom Combobox Implementation
#include "Utility/EditItem.h"           // For Custom Editbox Implementation
#include "Utility/NumSpinCtrl.h"        // For the custom spin control
#include "Utility/NumEdit.h"            // For Custom Numeric Edit control Impl
#include "Utility/FFListctrl.h"         // For Flicker Free List class definition
#include "Utility/Utility.h"
#include "FlexListCtrl.h"       // For editable list control implementation
#include "SignalDetailsDlg.h"   // for displaying the signal details
#include "TxMsgDetailsView.h"   // For CTxMsgDetailsView class declaration
#include "TxMsgChildFrame.h"    // For Parent window class declaration
// For DIL Interface Class
#include "Include/BaseDefs.h"
#include "DataTypes/Base_WrapperErrorLogger.h"
#include "DataTypes/MsgBufAll_Datatypes.h"
#include "DIL_Interface/BaseDIL_CAN.h"
#include "Utility/MultiLanguageSupport.h"

// Definition for image indices
#define defIMAGE_DIRTY 2
#define defIMAGE_GOOD  0

#define MAXCHECKSUMTYPE	2
const char checkSumTypeDef[5][10]= {
	{"Add"},
	{"XOR"}
};

/** Message name with ID attached in it MsgName[0xMsgID] */
UINT unGetMsgIDFromName(CString omMsgName)
{
    CString omStrMsgID;
    UINT unMsgID = (UINT)-1;
    CHAR* pcStopStr = nullptr;
    int nIndex = omMsgName.ReverseFind(defMSGID_EXTENDED);
    if(nIndex != -1)
    {
        int nLength = omMsgName.GetLength();
        omStrMsgID = omMsgName.Mid(nIndex+1,nLength-1);
        unMsgID = strtol((LPCTSTR )omStrMsgID,&pcStopStr,16);
    }
    return unMsgID;
}

IMPLEMENT_DYNCREATE(CTxMsgDetailsView, CFormView)

CTxMsgDetailsView::CTxMsgDetailsView() :
    CFormView(CTxMsgDetailsView::IDD),
    m_odSignalMatrix(8)
{
    m_bIsRTR = FALSE;
    m_nRBTNFrameFormat = 0;
    m_nChannelID = 0;
    m_psSelectedMsgDetails = nullptr;
    m_bIsMsgDirty = FALSE;

    // Clear Signal Matrix data structure
    memset(&m_unData, 0, sizeof( m_unData ));
    memset(&m_bData, 0, sizeof( m_bData ));
    m_pouDBPtr = nullptr;
}

CTxMsgDetailsView::~CTxMsgDetailsView()
{
}

void CTxMsgDetailsView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMB_CHANNEL_ID, m_omComboChannelID);
    DDX_Control(pDX, IDC_STAT_MATRIX_RECT, m_odSignalMatrix);
    DDX_Control(pDX, IDC_LSTC_SIG_DETAILS, m_omLctrSigList);
    DDX_Control(pDX, IDC_COMB_MSG_ID_NAME, m_omComboMsgIDorName);
    DDX_Control(pDX, IDC_CBTN_ADD_MSG, m_omButtonUpdateMsg);
    DDX_Control(pDX, IDC_EDIT_DLC, m_odDLC);
    DDX_Control(pDX, IDC_EDIT_DB8, m_odDB8);
    DDX_Control(pDX, IDC_EDIT_DB7, m_odDB7);
    DDX_Control(pDX, IDC_EDIT_DB6, m_odDB6);
    DDX_Control(pDX, IDC_EDIT_DB5, m_odDB5);
    DDX_Control(pDX, IDC_EDIT_DB4, m_odDB4);
    DDX_Control(pDX, IDC_EDIT_DB3, m_odDB3);
    DDX_Control(pDX, IDC_EDIT_DB2, m_odDB2);
    DDX_Control(pDX, IDC_EDIT_DB1, m_odDB1);
	DDX_Control(pDX, IDC_EDIT_ROLL_STARTBIT, m_omRollsb);
	DDX_Control(pDX, IDC_EDIT_ROLL_MAX, m_omRollmax);
	DDX_Control(pDX, IDC_EDIT_CHECKSUM_BYTE, m_chsumbyte);
	DDX_Control(pDX, IDC_COMBO_CHECKSUM_TYPE, m_omCheckSumType);
	DDX_Control(pDX, IDC_CHECK_ENROLLCHECK, m_enRollChk);
    DDX_Check(pDX, IDC_CHKB_MSGTYPE_RTR, m_bIsRTR);
    DDX_CBString(pDX, IDC_COMB_MSG_ID_NAME, m_omStrMsgIDorName);
    DDX_Radio(pDX, IDC_RBTN_MSGTYPE_STD, m_nRBTNFrameFormat);
    DDX_CBIndex(pDX, IDC_COMB_CHANNEL_ID, m_nChannelID);
}

BEGIN_MESSAGE_MAP(CTxMsgDetailsView, CFormView)
    ON_CBN_EDITCHANGE(IDC_COMB_MSG_ID_NAME, OnEditchangeCombMsgIdName)
    ON_CBN_SELCHANGE(IDC_COMB_MSG_ID_NAME, OnSelchangeCombMsgIdName)
    ON_EN_UPDATE(IDC_EDIT_DLC, OnUpdateEditDLC)
    ON_EN_UPDATE(IDC_EDIT_DB1, OnUpdateEditDataBytes)
    ON_BN_CLICKED(IDC_RBTN_MSGTYPE_STD, OnRbtnMsgtypeStd)
    ON_BN_CLICKED(IDC_CHKB_MSGTYPE_RTR, OnChkbMsgtypeRtr)
    ON_BN_CLICKED(IDC_CBTN_ADD_MSG, OnButtonAddMsg)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LSTC_SIG_DETAILS, OnItemchangedLstcSigDetails)
    ON_EN_UPDATE(IDC_EDIT_DB2, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB3, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB4, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB5, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB6, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB7, OnUpdateEditDataBytes)
    ON_EN_UPDATE(IDC_EDIT_DB8, OnUpdateEditDataBytes)
	ON_EN_UPDATE(IDC_EDIT_ROLL_STARTBIT, OnUpdateEditDataBytes)
	ON_EN_UPDATE(IDC_EDIT_ROLL_MAX, OnUpdateEditDataBytes)
	ON_EN_UPDATE(IDC_EDIT_CHECKSUM_BYTE, OnUpdateEditDataBytes)
	ON_EN_UPDATE(IDC_CHECK_ENROLLCHECK, OnUpdateEditDataBytes)
    ON_BN_CLICKED(IDC_RBTN_MSGTYPE_EXTD, OnRbtnMsgtypeStd)
    ON_CBN_SELCHANGE(IDC_COMB_CHANNEL_ID, OnSelchangeCombChannelId)
    ON_EN_KILLFOCUS(IDC_EDIT_DLC, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB1, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB2, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB3, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB4, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB5, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB6, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB7, vAutoUpdateModifyChanges)
    ON_EN_KILLFOCUS(IDC_EDIT_DB8, vAutoUpdateModifyChanges)
	ON_BN_CLICKED(IDC_CHECK_ENROLLCHECK, OnBnClickedCheckEnrollcheck)
	ON_CBN_SELCHANGE(IDC_COMBO_CHECKSUM_TYPE, OnCbnSelchangeComboChecksumType)
END_MESSAGE_MAP()

void CTxMsgDetailsView::OnInitialUpdate()
{
    // Call Parent function to do init
    CFormView::OnInitialUpdate();

    // Initialise window pointer in the Tx child window
    CTxMsgChildFrame* pomChildFrame =
        (CTxMsgChildFrame* )pomGetParentWindow();
    ;
    if( pomChildFrame != nullptr )
    {
        pomChildFrame->vSetTxMsgViewPointers( eTxMsgMessageDetailsView, this );
    }

    // Set Limit and Type for edit boxes
    vSetControlProperties();
    // Populate combobox with database messages
    vPopulateMessageComboBox();

    vUpdateChannelIDInfo();

    // Set Default values for Data bytes and othe CAN attributes
    vSetDefaultValues();
    // Init Signal List control
    vInitSignalListCtrl();
    // Clear the matrix
    m_odSignalMatrix.vResetValues();
    m_odSignalMatrix.vSetMessageLength(0);
    // Set Lable to indicate Mode
    CString omStrText = CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) ? _(defSTR_HEX_MODE) : _(defSTR_DEC_MODE);
    CWnd* pomLabel = GetDlgItem(IDC_STAT_HEADER2);
    if( pomLabel != nullptr )
    {
        // Set the text with Hex/Dec mode
        pomLabel->SetWindowText(omStrText);
    }
    // Set Frame Format to either std or exd
    if(m_nRBTNFrameFormat == -1 )
    {
        CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_CHECKED);
        CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_UNCHECKED);
    }
    else
    {
        CheckDlgButton(IDC_RBTN_MSGTYPE_STD,!m_nRBTNFrameFormat);
        CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,m_nRBTNFrameFormat);
    }
    UpdateData(TRUE);
    vUpdateStateDataBytes();
    // Disable Add Button
    m_omButtonUpdateMsg.EnableWindow( FALSE );
}

void CTxMsgDetailsView::vUpdateChannelIDInfo()
{
    // Update Channel ID Combobox
    m_omComboChannelID.ResetContent();

    LONG lParam = 0;
    if(((CBaseDIL_CAN*)CTxMsgManager::pGetDILInterfacePtr()) != nullptr)
    {
        if(((CBaseDIL_CAN*)CTxMsgManager::pGetDILInterfacePtr())
                ->DILC_GetControllerParams(lParam, 0, NUMBER_HW) == S_OK)
        {
            UINT nHardware = (UINT)lParam;

            for( int nIndex = 0; (UINT)nIndex < nHardware; nIndex++)
            {
                CString omStr;
                // Format Channel ID String Say 1,2,...
                omStr.Format( defFORMAT_MSGID_DECIMAL,
                              nIndex + 1 );
                // Add Channel ID
                m_omComboChannelID.AddString( omStr );
            }
        }
    }
    m_omComboChannelID.SetCurSel(0);
}

void CTxMsgDetailsView::vSetControlProperties()
{
    // Temp base holder
    INT nTempBase = -1;
    // Limit number of character input
    UINT unNumberOfChars = 0;

    // Set the control properties
    // to accept data in HEX if the base mode is HEXADECIMAL
    if (TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX))
    {
        nTempBase = BASE_HEXADECIMAL;
        unNumberOfChars = 2;
    }
    else
    {
        nTempBase = BASE_DECIMAL;
        unNumberOfChars = 3;
    }

    // Set base for Data bytes
    m_odDB1.vSetBase(nTempBase);
    m_odDB2.vSetBase(nTempBase);
    m_odDB3.vSetBase(nTempBase);
    m_odDB4.vSetBase(nTempBase);
    m_odDB5.vSetBase(nTempBase);
    m_odDB6.vSetBase(nTempBase);
    m_odDB7.vSetBase(nTempBase);
    m_odDB8.vSetBase(nTempBase);
	//m_omRollsb.vSetBase(nTempBase);
	m_omRollmax.vSetBase(nTempBase);
	m_chsumbyte.vSetBase(nTempBase);

    // Base for DLC
    m_odDLC.vSetBase(BASE_DECIMAL);

    // Set number of characters for input
    //  DLC
    m_odDLC.SetLimitText(1);
    // Databytes
    m_odDB1.SetLimitText(unNumberOfChars);
    m_odDB2.SetLimitText(unNumberOfChars);
    m_odDB3.SetLimitText(unNumberOfChars);
    m_odDB4.SetLimitText(unNumberOfChars);
    m_odDB5.SetLimitText(unNumberOfChars);
    m_odDB6.SetLimitText(unNumberOfChars);
    m_odDB7.SetLimitText(unNumberOfChars);
    m_odDB8.SetLimitText(unNumberOfChars);
	m_omRollsb.SetLimitText(unNumberOfChars);
	m_omRollmax.SetLimitText(unNumberOfChars);
	m_chsumbyte.SetLimitText(unNumberOfChars);

    // Set all the edit controls
    // to accept unsigned numbers
    m_odDLC.vSetSigned( FALSE );
    // Databytes
    m_odDB1.vSetSigned( FALSE );
    m_odDB2.vSetSigned( FALSE );
    m_odDB3.vSetSigned( FALSE );
    m_odDB4.vSetSigned( FALSE );
    m_odDB5.vSetSigned( FALSE );
    m_odDB6.vSetSigned( FALSE );
    m_odDB7.vSetSigned( FALSE );
    m_odDB8.vSetSigned( FALSE );
	//m_omRollsb.vSetSigned(FALSE);
	m_omRollmax.vSetSigned(FALSE);
	m_chsumbyte.vSetSigned(FALSE);
	if (m_omCheckSumType.GetCount() == 0)
	{	
		for (int i=0; i < MAXCHECKSUMTYPE; i++)
		{
			m_omCheckSumType.AddString(_T(checkSumTypeDef[i]));
		}
	}

	//m_omCheckSumType.set
	//m_omCheckSumType.SetCueBanner(_T("Select an item..."));
	
}

void CTxMsgDetailsView::vInitSignalListCtrl()
{
    CHAR caColumnName[][defSIGNAL_LIST_STRING_MAX_LENGTH]
        = { defSTR_COL_SIGNAL_NAME,
            defSTR_RAW_COLUMN,
            defSTR_PHYSICAL_COLUMN,
            defSTR_SIGNAL_UNIT
          };
    INT nColumnFormat[]     = { LVCFMT_LEFT,
                                LVCFMT_CENTER,
                                LVCFMT_CENTER,
                                LVCFMT_LEFT
                              };
    RECT rListCtrlRect;
    INT nTotalColunmSize = 0;
    INT nTotalStrLengthPixel = 0;
    INT nColumnSize = 0;
    // Set the Image List
    if( m_omSigImageList.m_hImageList == nullptr )
    {
        m_omSigImageList.Create( IDR_BMP_MSG_SIG_WATCH, 16,
                                 1, defCOLOR_WHITE );
    }
    m_omLctrSigList.SetImageList(&m_omSigImageList, LVSIL_SMALL);

    //Calculate the total size of all column header
    m_omLctrSigList.GetWindowRect( &rListCtrlRect);
    nTotalColunmSize     = rListCtrlRect.right - rListCtrlRect.left;
    nTotalStrLengthPixel = 0;

    UINT i; //i declared outside loop
    for(i=0; i<defSIGNAL_FRAME_COLUMN; i++)
    {
        nTotalStrLengthPixel +=
            m_omLctrSigList.GetStringWidth(_(caColumnName[i])) ;
    }
    //Insert each column name after calculating the size for the same.
    for(i=0; i<defSIGNAL_FRAME_COLUMN; i++)
    {
        nColumnSize  = m_omLctrSigList.GetStringWidth(_(caColumnName[i])) ;
        nColumnSize +=
            (nTotalColunmSize-nTotalStrLengthPixel)/defSIGNAL_FRAME_COLUMN;
        m_omLctrSigList.InsertColumn(i, _(caColumnName[i]),
                                     nColumnFormat[i], nColumnSize - 1);
    }

    // Set the extended style
    m_omLctrSigList.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    SNUMERICINFO    sNumInfo;
    SLISTINFO       sListInfo;
    SUSERPROGINFO   sProgInfo;

    // Init Signal Name column
    // Column 0 : User Function call
    sListInfo.m_eType = eUser;
    m_omLctrSigList.vSetColumnInfo(0, 0, sListInfo);
    sProgInfo.m_pfHandler = vSignalNameHandler;
    sProgInfo.m_pUserParam = this;
    m_omLctrSigList.vSetUserProgInfo(0, 0, sProgInfo);

    // Init Raw Value column
    // Column 1 : Numeric Edit with Spin Control
    sListInfo.m_eType = eBuddy;
    m_omLctrSigList.vSetColumnInfo(0, 1, sListInfo);

    // Init Phy Value column
    // Column 2 : Numeric Edit with Spin Control
    sListInfo.m_eType = eBuddy;
    m_omLctrSigList.vSetColumnInfo(0, 2, sListInfo);

    // Init Unit column
    // Column 3 : Numeric Edit with Spin Control
    // Init is not required as it non-editable column


    // Disable Signal List as there is no selected message at this time
    m_omLctrSigList.EnableWindow( FALSE );
    // Signal Matrix is not created at this point of time
    // Disable that after create
}

void CTxMsgDetailsView::vSignalNameHandler( CListCtrl*, //pList,
        int nItem,
        int nSubItem,
        void* uParam)
{
    // Signal Name Click
    // Popup Signal Info Dialog
    if( nSubItem == 0 )
    {
        // Get the dialog pointer
        CTxMsgDetailsView* podDlg = nullptr;
        podDlg = ( CTxMsgDetailsView* )uParam;
        if( podDlg != nullptr )
        {
            sSIGNALS* psSignal = nullptr;
            // Get the signal Information
            psSignal = podDlg->psGetSelectedSignalStruct( nItem );
            if( psSignal != nullptr )
            {
                // Show Signal Details dialog
                CSignalDetailsDlg odSigDetails( MD_READ_ONLY, psSignal, podDlg);
                odSigDetails.DoModal();
            }
        }
    }
}

sSIGNALS* CTxMsgDetailsView::psGetSelectedSignalStruct(int nIndex)
{
    // Init the pointer
    sSIGNALS* psSignal = nullptr;
    // If the local copy is valid
    if( m_psSelectedMsgDetails != nullptr )
    {
        BOOL bStop = FALSE;
        // Get the selected signal information
        CString omStrSigName =
            m_omLctrSigList.GetItemText(nIndex, 0);
        // Get the Signal List head pointer
        psSignal = m_psSelectedMsgDetails->m_psSignals;
        // Iterate through the list
        while( psSignal != nullptr && bStop == FALSE)
        {
            // Compare the signal name
            if( omStrSigName.Compare(psSignal->m_omStrSignalName) == 0 )
            {
                // If found then stop traversing
                bStop = TRUE;
            }
            // Go to next node
            else
            {
                psSignal = psSignal->m_psNextSignalList;
            }
        }
    }
    // Return valid pointer or nullptr
    return psSignal;
}

void CTxMsgDetailsView::vSetMsgDBPtr(void* pMsgDB)
{
    m_pouDBPtr = (CMsgSignal*)pMsgDB;
}

void CTxMsgDetailsView::OnEditchangeCombMsgIdName()
{
    BOOL bIDValid = FALSE;
    INT nMsgID = -1;
    // Get the message ID.
    nMsgID = nGetMessageID();
    // If it is valid
    if(nMsgID != -1 )
    {
        UpdateData(TRUE);
        // Check for valid message ID
        if(m_nRBTNFrameFormat == TRUE && nMsgID < defMAX_LMT_EXTD_MSG_ID )
        {
            bIDValid = TRUE;
        }
        if(m_nRBTNFrameFormat == FALSE && nMsgID < MAX_LMT_FOR_STD_MSG_ID)
        {
            bIDValid = TRUE;
        }
    }
    if( bIDValid == TRUE )
    {
        // Update Message List Information
        STCAN_MSG sCanInfo;
        // Reset the status text
        bSetStatusText( "" );
        // Update the message details from the UI
        bUpdateMessageDetail(&sCanInfo);
        // Set the message length
        m_odSignalMatrix.vSetMessageLength(sCanInfo.m_ucDataLen);
        //If it is a database message then update Signal List
        sMESSAGE* psMsg = nullptr;
        // Check if it is a database message ID/Name
        psMsg = m_pouDBPtr->psGetMessagePointer(nMsgID);
        // Update signal List
        if( psMsg != nullptr )
        {
            CString omStrMsgName = "";
            // Update DLC Field
            omStrMsgName.Format( defFORMAT_MSGID_DECIMAL,
                                 psMsg->m_unMessageLength );
            m_odDLC.SetWindowText( omStrMsgName);
            // Update Message Frame Type
            if(psMsg->m_bMessageFrameFormat == TRUE)
            {
                CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_CHECKED);
                CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_UNCHECKED);
            }
            else
            {
                CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_UNCHECKED);
                CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_CHECKED);
            }
            // Update Signal List and Matrix
            bUpdateSignalList(sCanInfo);
        }
        else
        {
            // For non database message ID
            // Remove All signals from the lsit
            m_omLctrSigList.DeleteAllItems();
            // Disable it
            m_omLctrSigList.EnableWindow( FALSE );
            // Set the message Length
            if( sCanInfo.m_ucRTR == FALSE)
            {
                m_odSignalMatrix.vSetMessageLength(sCanInfo.m_ucDataLen);
                // Set the data value
                m_odSignalMatrix.vSetByteValue( sCanInfo.m_ucData );
            }
            else
            {
                // For RTR Messages
                m_odSignalMatrix.vSetMessageLength(0);
            }
        }
        // Enable Add Button
        vEnableAddButton( TRUE );
        // Update the list control
        vUpdateSelectedMessageDetails();
        vUpdateStateDataBytes();

        //update the global list for storing the changed data
        vCallApplyChanges();
    }
    else
    {
        // Disable Add Button
        vEnableAddButton( FALSE );
        // Show Error Text
        bSetStatusText( _(defSTR_INVALID_MESSAGE_ID) );
        // Disable Signal Matrix
        m_odSignalMatrix.vSetMessageLength(0);
        // For invalid message ID
        // Remove All signals from the lsit
        m_omLctrSigList.DeleteAllItems();
        // Disable it
        m_omLctrSigList.EnableWindow( FALSE );
    }
}

BOOL CTxMsgDetailsView::bSetStatusText(const char* pStrText)
{
    BOOL bSuccess = FALSE;
    if( pStrText != nullptr )
    {
        CWnd* pWnd = GetDlgItem(IDC_STAT_STATUS);
        if( pWnd != nullptr )
        {
            pWnd->SetWindowText(pStrText);
            bSuccess = TRUE;
        }
    }
    return bSuccess;
}

void CTxMsgDetailsView::vUpdateSignalMatrix(void)
{
    // Set the bit matrix
    for ( int index = 0; index < defMAX_BYTE; index++)
    {
        int nBit = index * defBITS_IN_BYTE;
        m_bData[nBit + 0] = m_unData[index].sByte.Bit0;
        m_bData[nBit + 1] = m_unData[index].sByte.Bit1;
        m_bData[nBit + 2] = m_unData[index].sByte.Bit2;
        m_bData[nBit + 3] = m_unData[index].sByte.Bit3;
        m_bData[nBit + 4] = m_unData[index].sByte.Bit4;
        m_bData[nBit + 5] = m_unData[index].sByte.Bit5;
        m_bData[nBit + 6] = m_unData[index].sByte.Bit6;
        m_bData[nBit + 7] = m_unData[index].sByte.Bit7;
    }

    // Set the data value
    m_odSignalMatrix.vSetValue( m_bData );
    // Update Signal Matrix List
    m_odSignalMatrix.Invalidate();
}

BOOL CTxMsgDetailsView::bUpdateMessageDetail(STCAN_MSG* psMsgDetails)
{
    BOOL bReturn = TRUE;
    UpdateData(TRUE);
    vUpdateStateDataBytes();
    psMsgDetails->m_unMsgID    = nGetMessageID();

    psMsgDetails->m_ucDataLen  = static_cast<UCHAR>(m_odDLC.lGetValue());

    psMsgDetails->m_ucData[0]  = static_cast<UCHAR>(m_odDB1.lGetValue());
    psMsgDetails->m_ucData[1]  = static_cast<UCHAR>(m_odDB2.lGetValue());
    psMsgDetails->m_ucData[2]  = static_cast<UCHAR>(m_odDB3.lGetValue());
    psMsgDetails->m_ucData[3]  = static_cast<UCHAR>(m_odDB4.lGetValue());
    psMsgDetails->m_ucData[4]  = static_cast<UCHAR>(m_odDB5.lGetValue());
    psMsgDetails->m_ucData[5]  = static_cast<UCHAR>(m_odDB6.lGetValue());
    psMsgDetails->m_ucData[6]  = static_cast<UCHAR>(m_odDB7.lGetValue());
    psMsgDetails->m_ucData[7]  = static_cast<UCHAR>(m_odDB8.lGetValue());

    psMsgDetails->m_ucRTR      = static_cast<UCHAR>(m_bIsRTR);
    psMsgDetails->m_ucEXTENDED = static_cast<UCHAR>(m_nRBTNFrameFormat);

    // Update Channel ID
    psMsgDetails->m_ucChannel = (UCHAR)m_omComboChannelID.GetCurSel() + 1;
    return bReturn;
}
BOOL CTxMsgDetailsView::bUpdateMessageRollCntCheckSum(void)
{

	//update the global list for storing the changed data******************
	//int             nCurrentIndex = -1;
	int startbit,rollmax,checkbyte;
	CTxMsgBlocksView* pomBlockView = nullptr;
	CTxMsgListView* pomListView = nullptr;
	pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
	pomListView = (CTxMsgListView* )pomGetListViewPointer();

	PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
	// Get current block pointer
	psMsgCurrentBlock =
		pomBlockView->psGetMsgBlockPointer(
		pomBlockView->m_nSelectedMsgBlockIndex,
		pomBlockView->m_psMsgBlockList );

	if(psMsgCurrentBlock != nullptr)
	{
		PSTXCANMSGLIST pCanMsgList = psMsgCurrentBlock->m_psTxCANMsgList;

		pCanMsgList =  pomListView->psGetMsgDetailPointer(
			pomListView->m_nSelectedMsgIndex, psMsgCurrentBlock);
		if(pCanMsgList != nullptr)
		{
			CString str;
			startbit = static_cast<UCHAR>(m_omRollsb.lGetValue());
			rollmax = static_cast<UCHAR>(m_omRollmax.lGetValue());
			checkbyte = static_cast<UCHAR>(m_chsumbyte.lGetValue());
#if 1
			if (checkbyte > 7 || checkbyte < 0)
			{
				AfxMessageBox("The checksum byte must between 0-7,check your input!");
				checkbyte = 7;
			}
			if((startbit) > 63 || startbit < 0)
			{
				AfxMessageBox("The rolling counter start bit must between 0-63,check your input!");
				startbit = 0;
			}
			int i = rollmax;
			int rollneddbits = 0,rollavailbits=0;
			while (i)
			{
				i = i>>1;
				rollneddbits++;
			}
			rollavailbits = (startbit >= 8)?(8-(startbit%8)):9-startbit;
			if(rollneddbits > rollavailbits )
			{
				
				AfxMessageBox("The rolling counter max is too big for the start bit!");
				rollmax = 0;
			}
#endif
			pCanMsgList->m_bModified = true;
			pCanMsgList->m_rollCntStartBit = startbit;
			pCanMsgList->m_rollCntMax = rollmax;
			pCanMsgList->m_checkSumByte = checkbyte;
			pCanMsgList->m_checkSumType = m_omCheckSumType.GetCurSel();
			pCanMsgList->m_bEnRollCntCheck = m_enRollChk.GetCheck();
		}
	}
	return TRUE;
}
BOOL CTxMsgDetailsView::bUpdateSignalList(STCAN_MSG sMsg)
{
    CMsgSignal* podDatabase = nullptr;

    BOOL bSuccess = FALSE;
    // Get the database Pointer
    podDatabase = m_pouDBPtr;
    if( podDatabase != nullptr )
    {
        // Save the selected message information
        // This should be cleared on database import
        m_psSelectedMsgDetails =
            podDatabase->psGetMessagePointer( sMsg.m_unMsgID);
        if( m_psSelectedMsgDetails != nullptr && sMsg.m_ucRTR == FALSE)
        {
            CMsgInterpretation  odMsgInterpet;
            CSignalInfoArray    omSigInfo;
            bSuccess = odMsgInterpet.bInterpretMsgs(DEC,  m_psSelectedMsgDetails,
                                                    sMsg.m_ucData,
                                                    omSigInfo);
            if( bSuccess == TRUE)
            {
                // Enable the list control
                m_omLctrSigList.EnableWindow();
                vShowSignalValues( omSigInfo);
                if( m_odSignalMatrix.m_hWnd != nullptr )
                {
                    // Reset the values
                    m_odSignalMatrix.vResetValues();
                    // Set the message Length
                    m_odSignalMatrix.vSetMessageLength(sMsg.m_ucDataLen);

                    //int nMsgLen = psSelectedMsgDetails->m_unMessageLength;
                    // Use actual message length
                    int nMsgLen = sMsg.m_ucDataLen;
                    if(m_psSelectedMsgDetails->m_nMsgDataFormat != DATA_FORMAT_INTEL)
                    {
                        for( int index = 0; index < nMsgLen; index++)
                        {
                            m_unData[ index ].byByte =
                                sMsg.m_ucData[ nMsgLen - index - 1];
                        }
                    }
                    else
                    {
                        for( int index = 0; index < nMsgLen; index++)
                        {
                            m_unData[ index ].byByte = sMsg.m_ucData[ index ];
                        }
                    }
                    // Set the bit matrix
                    for ( int index = 0; index < nMsgLen; index++)
                    {
                        int nBit = index * defBITS_IN_BYTE;
                        m_bData[nBit + 0] = m_unData[ index ].sByte.Bit0;
                        m_bData[nBit + 1] = m_unData[ index ].sByte.Bit1;
                        m_bData[nBit + 2] = m_unData[ index ].sByte.Bit2;
                        m_bData[nBit + 3] = m_unData[ index ].sByte.Bit3;
                        m_bData[nBit + 4] = m_unData[ index ].sByte.Bit4;
                        m_bData[nBit + 5] = m_unData[ index ].sByte.Bit5;
                        m_bData[nBit + 6] = m_unData[ index ].sByte.Bit6;
                        m_bData[nBit + 7] = m_unData[ index ].sByte.Bit7;
                    }

                    // Set the data value
                    m_odSignalMatrix.vSetValue( m_bData );
                    // Update Signal Matrix List
                    m_odSignalMatrix.Invalidate();
                }
            }
        }
        //      else
        if(!bSuccess)//no signal found.
        {
            // Non Database Message
            // ASSERT( FALSE );
            // Clear Signal List
            m_omLctrSigList.DeleteAllItems();
            // Disable Signal List
            m_omLctrSigList.EnableWindow(FALSE);

            if( m_odSignalMatrix.m_hWnd != nullptr )
            {
                // Reset the values
                m_odSignalMatrix.vResetValues();
                // Set the message Length
                if( sMsg.m_ucRTR == FALSE)
                {
                    m_odSignalMatrix.vSetMessageLength(sMsg.m_ucDataLen);
                    // Set the data value
                    m_odSignalMatrix.vSetByteValue( sMsg.m_ucData );
                }
                else
                {
                    // For RTR Messages
                    m_odSignalMatrix.vSetMessageLength(0);
                }
            }
        }
    }
    else
    {
        // Database should not be an empty one
        ASSERT( FALSE );
    }
    vUpdateStateDataBytes();

    return bSuccess;
}

INT CTxMsgDetailsView::nGetMessageID()
{
    CString omStrMsgName( "" );
    INT nMsgID = -1;
    m_omComboMsgIDorName.GetWindowText(omStrMsgName);
    // Get message Id from database in case user has selected a message name.
    //if it is name [] is present
    int nIndex = omStrMsgName.Find(defMSGID_NAME_START_CHAR);
    if(nIndex != -1)
    {
        nMsgID = unGetMsgIDFromName(omStrMsgName);
    }
    // If the message name is not selected then get it from edit box and convert
    // it to the integer value.
    else
    {
        UINT unMsgID = 0;
        CHAR* pcStr = nullptr;
        BOOL bHex = TRUE;
        bHex = CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX);
        if(bHex == TRUE )
        {
            unMsgID = strtol(omStrMsgName,&pcStr,16);
        }
        else
        {
            unMsgID = strtol(omStrMsgName,&pcStr,10);
        }
        // check if the field is empty or user have entered "0".
        // so in case user has entered "0" or if non zero value is
        // entered and strtol function has writtened correct value.
        if(omStrMsgName.CompareNoCase("0") != 0 && unMsgID != 0 ||
                omStrMsgName.CompareNoCase("0") == 0 )
        {
            nMsgID = static_cast<INT> (unMsgID);
        }
    }
    vUpdateStateDataBytes();

    return nMsgID;
}

void CTxMsgDetailsView::vEnableAddButton(BOOL bEnable)
{
    // Get Block View Pointer
    CTxMsgBlocksView* pomBlocksView = nullptr;
    pomBlocksView = ( CTxMsgBlocksView*)pomGetBlocksViewPointer();
    if( pomBlocksView != nullptr )
    {
        // This local variable with FALSE will avoid lots of elase blocks
        BOOL bEnaleAddButton = FALSE;
        // If it is Enable
        if( bEnable == TRUE )
        {
            // Get the message ID
            int nMsgID = nGetValidMessageID();
            // If it is valid
            if( nMsgID != -1)
            {
                // Get DLC value
                int nDLC =  (int)(m_odDLC.lGetValue());
                // If DLC Text is empty the above function call will return
                // 0. So make it -1 to denote invalid value
                if( nDLC == 0 )
                {
                    CString omStrText;
                    m_odDLC.GetWindowText(omStrText);
                    if( omStrText.IsEmpty() == TRUE )
                    {
                        nDLC = -1;
                    }
                }
                // If it is valid
                if( nDLC >= 0 && nDLC < 9 )
                {
                    PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
                    psMsgCurrentBlock =
                        pomBlocksView->psGetMsgBlockPointer(
                            pomBlocksView->m_nSelectedMsgBlockIndex,
                            pomBlocksView->m_psMsgBlockList );
                    if(psMsgCurrentBlock != nullptr )
                    {
                        if(psMsgCurrentBlock->m_unMsgCount < defMAX_MSGINBLOCK )
                        {
                            bEnaleAddButton = TRUE;
                        }
                    }
                }
            }
        }
        // Else is not required as we can directly disable button
        m_omButtonUpdateMsg.EnableWindow ( bEnaleAddButton );

        // Enable Apply Button
        CTxFunctionsView* pView =
            ( CTxFunctionsView* )pomGetFunctionsViewPointer();
        if( pView != nullptr )
        {
            if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
            {
                pView->m_omButtonApply.EnableWindow( TRUE );
            }
        }
    }
}

BOOL CTxMsgDetailsView::vUpdateSelectedMessageDetails()
{
    BOOL bDataValid = FALSE;
    // Get Other view pointers
    CTxMsgBlocksView* pomBlockView = nullptr;
    CTxMsgListView* pomListView = nullptr;
    pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    if( pomBlockView != nullptr && pomListView != nullptr )
    {
        bDataValid = bValidateData();
        if(bDataValid == TRUE )
        {
            // make it dirty in case database message is selected "*" will be
            // added at the end of databytes.
            //m_bIsMsgDirty = TRUE;
            if(pomListView->m_nSelectedMsgIndex != -1)
            {
                PSMSGBLOCKLIST psMsgBlock  = nullptr;
                PSTXCANMSGLIST psTxMsgList = nullptr;
                psMsgBlock     =
                    pomBlockView->psGetMsgBlockPointer(
                        pomBlockView->m_nSelectedMsgBlockIndex,
                        pomBlockView->m_psMsgBlockList );
                if(psMsgBlock != nullptr )
                {
                    psTxMsgList = pomListView->psGetMsgDetailPointer(
                                      pomListView->m_nSelectedMsgIndex,
                                      psMsgBlock );

                    if(psTxMsgList != nullptr)
                    {
                        psTxMsgList->m_sTxMsgDetails.m_bIsMsgDirty =
                            m_bIsMsgDirty;
                        bUpdateMessageDetail(&(psTxMsgList->
                                               m_sTxMsgDetails.m_sTxMsg));
                        pomListView->vUpdateMsgListDisplay(
                            psTxMsgList->m_sTxMsgDetails,
                            pomListView->m_nSelectedMsgIndex );
                    }
                }
            }
        }
        // Apply changes only on press of apply button
        // So comment the code
        // Only Enable Apply Button if the data is valid
        if( bDataValid == TRUE)
        {
            //vApplyChanges();
            // Enable Apply Button too
            CTxFunctionsView* pView =
                ( CTxFunctionsView* )pomGetFunctionsViewPointer();
            if( pView != nullptr )
            {
                if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                {
                    pView->m_omButtonApply.EnableWindow( TRUE );
                }
            }
        }
    }
    return bDataValid;
}

void CTxMsgDetailsView::vShowSignalValues(const CSignalInfoArray& romSigInfo)
{
    // Get Other view pointers
    CTxMsgBlocksView* pomBlockView = nullptr;
    CTxMsgListView* pomListView = nullptr;
    pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    if( pomBlockView != nullptr && pomListView != nullptr )
    {
        // Number of Signals in the list
        int nItems = (INT)romSigInfo.GetSize();
        // Set the flag to indicate data update
        m_bUpdating = TRUE;
        // Existing the list items count
        int nExistingItems = m_omLctrSigList.GetItemCount();
        // Get Message Block Pointer
        PSMSGBLOCKLIST psMsgBlock  = nullptr;
        PSTXCANMSGLIST psTxMsgList = nullptr;

        int nImageIndex = 0;
        // Get the message Block details
        psMsgBlock = pomBlockView->psGetMsgBlockPointer(
                         pomBlockView->m_nSelectedMsgBlockIndex,
                         pomBlockView->m_psMsgBlockList );
        if(psMsgBlock != nullptr )
        {
            // Get the message details from the block
            psTxMsgList = pomListView->psGetMsgDetailPointer(
                              pomListView->m_nSelectedMsgIndex,
                              psMsgBlock );
            if(psTxMsgList != nullptr)
            {
                // Set the Image index approp.
                nImageIndex = psTxMsgList->m_sTxMsgDetails.m_bIsMsgDirty ?
                              defIMAGE_DIRTY : defIMAGE_GOOD;
            }
        }

        // Fill Item Structure with image index
        LVITEM sItem;
        sItem.mask      = LVIF_IMAGE;
        sItem.iSubItem  = defMAIN_ITEM;
        sItem.iImage    = nImageIndex;
        // If reuse entries. Add/Delete if the previous entry count is different
        if( nExistingItems!= nItems )
        {
            // Add Additional entries
            if( nExistingItems < nItems )
            {
                for(int index = nExistingItems; index < nItems; index++)
                {
                    m_omLctrSigList.InsertItem( index, "");
                }
            }
            // Remove extra entries
            else
            {
                for(int index = nExistingItems - 1; index >= nItems; index--)
                {
                    m_omLctrSigList.DeleteItem(index);
                }
            }
        }
        // Remove the selection
        int nSelIndex = m_omLctrSigList.GetNextItem(-1, LVNI_SELECTED);
        if( nSelIndex != -1 )
        {
            m_omLctrSigList.SetItemState( nSelIndex,
                                          0,
                                          LVIS_SELECTED | LVIS_FOCUSED );
        }

        SINTERPRETSIGNALINFO sSignalInfo;
        CString omStrFormat;
        SUSERPROGINFO sProgInfo;
        SNUMERICINFO   sNumInfo;
        SLISTINFO      sColType;

        // Init User Program information. This is required to show signal
        // details on double click of Signal Name
        sProgInfo.m_pfHandler = vSignalNameHandler;
        // Pass this as suer parameter
        sProgInfo.m_pUserParam = this;

        // update signal information
        for( int nIndex = 0; nIndex < nItems; nIndex++)
        {
            // Get the current signal
            sSignalInfo = romSigInfo[ nIndex ];
            // Set the item as the row is already inserted
            m_omLctrSigList.SetItemText( nIndex, 0,
                                         sSignalInfo.m_omStrSignalName );
            // Set the list row image
            sItem.iItem     = nIndex;
            m_omLctrSigList.SetItem(&sItem);
            // Update Phy and Raw values
            if( m_psSelectedMsgDetails != nullptr)
            {
                // Find the signal Pointer
                sSIGNALS* psSignal =
                    m_psSelectedMsgDetails->m_psSignals;
                while( psSignal != nullptr )
                {
                    if( psSignal->m_omStrSignalName.Compare(
                                sSignalInfo.m_omStrSignalName ) == 0 )
                    {
                        break;
                    }
                    else
                    {
                        psSignal = psSignal->m_psNextSignalList;
                    }
                }
                // If signal information found
                if( psSignal != nullptr )
                {
                    // Int with Signal Descriptor

                    // Check for Signal descriptor first
                    if( psSignal->m_oSignalIDVal != nullptr )
                    {
                        BOOL bFound = FALSE;
                        // If signal descriptor found then it is a editable
                        // combo box type
                        sColType.m_eType = eComboList;
                        sColType.m_omEntries.RemoveAll();
                        CSignalDescVal* pomDS = psSignal->m_oSignalIDVal;
                        // Add all descriptor entries in to the combo box
                        while( pomDS != nullptr)
                        {
                            sColType.m_omEntries.Add(
                                pomDS->m_omStrSignalDescriptor );
                            // Use signal descriptor value as current physical
                            //                          if( sSignalInfo.m_un64RawValue ==
                            //                              pomDS->m_n64SignalVal )
                            if( sSignalInfo.m_un64RawValue ==
                                    pomDS->m_DescValue.un64Value)
                            {
                                omStrFormat = pomDS->m_omStrSignalDescriptor;
                                bFound = TRUE;
                            }
                            pomDS = pomDS->m_pouNextSignalSignalDescVal;
                        }
                        // If the current raw value is not in the SD table
                        // Use Factor-Offset to calculate physical value
                        if( bFound == FALSE )
                        {
                            omStrFormat.Format( defSTR_FORMAT_PHY_VALUE,
                                                sSignalInfo.m_dPhyValue);
                        }
                        // Set the physical value col type
                        m_omLctrSigList.vSetColumnInfo( nIndex,
                                                        def_PHY_VALUE_COLUMN,
                                                        sColType );
                        // Set the physical value
                        m_omLctrSigList.SetItemText( nIndex,
                                                     def_PHY_VALUE_COLUMN,
                                                     omStrFormat );
                    }
                    // With out Signal Descriptor
                    else
                    {
                        // If the signal descriptor is not there
                        // Then use numeric edit with buddy control
                        sNumInfo.m_byFlag = BUDDY_CTRL;
                        // Physical value should be in dec always
                        sNumInfo.m_byBase = defBASE_DEC;
                        // Set Data Length
                        sNumInfo.m_nSigLength = (short)psSignal->m_unSignalLength;
                        // For Signed int
                        if( psSignal->m_bySignalType == CHAR_INT)
                        {
                            sNumInfo.m_byFlag |= SIGNED_VAL;
                        }
                        // For floating point check
                        if( ((psSignal->m_fSignalFactor -
                                ((int) psSignal->m_fSignalFactor)) != 0.0f) ||
                                ((psSignal->m_fSignalOffset -
                                  ((int) psSignal->m_fSignalOffset)) != 0.0f ))
                        {
                            sNumInfo.m_byFlag |= FLOAT_VAL;
                        }

                        // Extend signed number to get Equivalent 64 bit value
                        __int64 n64Val = 0;
                        unsigned _int64 un64Val = 0;
                        n64Val = psSignal->m_SignalMinValue.n64Value;
                        if( IS_SIGNED_NUMBER( sNumInfo.m_byFlag ) )
                        {
                            s_vExtendSignBit( n64Val,psSignal->m_unSignalLength );
                        }
                        // Calculate the Min, Max and step values for the spin
                        // control
                        if ( IS_FLOAT_ENABLED( sNumInfo.m_byFlag) )
                        {
                            sNumInfo.m_uMinVal.m_dValue = n64Val *
                                                          psSignal->m_fSignalFactor +
                                                          psSignal->m_fSignalOffset;
                        }
                        else
                        {
                            sNumInfo.m_uMinVal.m_n64Value = __int64(n64Val *
                                                                    psSignal->m_fSignalFactor +
                                                                    psSignal->m_fSignalOffset);
                        }

                        if(psSignal->m_bySignalType == CHAR_INT)
                        {
                            n64Val = psSignal->m_SignalMaxValue.n64Value;
                        }
                        else
                        {
                            un64Val = psSignal->m_SignalMaxValue.un64Value;
                        }
                        // Check for negative sign
                        if( IS_SIGNED_NUMBER( sNumInfo.m_byFlag ) )
                        {
                            s_vExtendSignBit( n64Val, psSignal->m_unSignalLength);
                        }
                        // Max Val
                        if ( IS_FLOAT_ENABLED( sNumInfo.m_byFlag) )
                        {
                            if(psSignal->m_bySignalType == CHAR_INT)
                            {
                                sNumInfo.m_uMaxVal.m_dValue = psSignal->m_SignalMaxValue.n64Value *
                                                              psSignal->m_fSignalFactor +
                                                              psSignal->m_fSignalOffset;
                                // Step value = Factor value
                                sNumInfo.m_uDelta.m_dValue =
                                    psSignal->m_fSignalFactor;
                            }
                            else
                            {
                                sNumInfo.m_uMaxVal.m_dValue = psSignal->m_SignalMaxValue.un64Value *
                                                              psSignal->m_fSignalFactor +
                                                              psSignal->m_fSignalOffset;
                                // Step value = Factor value
                                sNumInfo.m_uDelta.m_dValue =
                                    psSignal->m_fSignalFactor;
                            }
                        }
                        else
                        {
                            if(psSignal->m_bySignalType == CHAR_INT)
                            {
                                sNumInfo.m_uMaxVal.m_n64Value = __int64(n64Val *
                                                                        psSignal->m_fSignalFactor +
                                                                        psSignal->m_fSignalOffset);
                                if(sNumInfo.m_uMaxVal.m_n64Value < 0)
                                {
                                    sNumInfo.m_uMaxVal.m_n64Value = n64Val;
                                }
                            }
                            else
                            {
                                sNumInfo.m_uMaxVal.m_un64Value = unsigned __int64(un64Val *
                                                                 psSignal->m_fSignalFactor +
                                                                 psSignal->m_fSignalOffset);
                            }
                            // Step value = Factor value
                            sNumInfo.m_uDelta.m_n64Value =
                                __int64(psSignal->m_fSignalFactor);
                        }
                        sColType.m_eType = eNumber;
                        // Set the physical value col type
                        m_omLctrSigList.vSetColumnInfo( nIndex,
                                                        def_PHY_VALUE_COLUMN,
                                                        sColType);
                        m_omLctrSigList.vSetNumericInfo( nIndex,
                                                         def_PHY_VALUE_COLUMN,
                                                         sNumInfo );
                        // Set the physical value
                        if( IS_FLOAT_ENABLED( sNumInfo.m_byFlag) )
                        {
                            omStrFormat.Format( defSTR_FORMAT_PHY_VALUE,
                                                sSignalInfo.m_dPhyValue);
                        }
                        else
                        {
                            if( psSignal->m_bySignalType == CHAR_INT)
                            {
                                omStrFormat.Format( defFMT_ZERO_PRECESSION,
                                                    sSignalInfo.m_dPhyValue);
                            }
                            else
                            {
                                omStrFormat.Format( defFORMAT_UINT64_DEC,
                                                    (UINT64)sSignalInfo.m_dPhyValue);
                            }
                        }
                        m_omLctrSigList.SetItemText( nIndex,
                                                     def_PHY_VALUE_COLUMN,
                                                     omStrFormat);
                    }
                    // Set Raw Value information
                    sColType.m_eType = eNumber;
                    sNumInfo.m_byFlag = BUDDY_CTRL;
                    // Care for Signed number. Extend the sign bit to get 64 bit
                    // Equivalent number
                    if( psSignal->m_bySignalType == CHAR_INT)
                    {
                        __int64 n64Val = 0;
                        sNumInfo.m_byFlag |= SIGNED_VAL;
                        n64Val = psSignal->m_SignalMinValue.n64Value;
                        s_vExtendSignBit( n64Val, psSignal->m_unSignalLength);
                        sNumInfo.m_uMinVal.m_n64Value = n64Val;

                        n64Val = psSignal->m_SignalMaxValue.n64Value;
                        s_vExtendSignBit( n64Val,psSignal->m_unSignalLength);
                        sNumInfo.m_uMaxVal.m_n64Value = n64Val;
                    }
                    // For unsigned number sign check is not required
                    else
                    {
                        sNumInfo.m_uMinVal.m_un64Value =
                            psSignal->m_SignalMinValue.un64Value;
                        sNumInfo.m_uMaxVal.m_un64Value =
                            psSignal->m_SignalMaxValue.un64Value;
                    }
                    // Set the step as 1.0 always
                    sNumInfo.m_uDelta.m_n64Value = 1;
                    // Set the data type length
                    sNumInfo.m_nSigLength = (short)psSignal->m_unSignalLength;
                    // set the base value
                    if( FALSE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) )
                    {
                        sNumInfo.m_byBase = defBASE_DEC;
                    }
                    else
                    {
                        sNumInfo.m_byBase = defBASE_HEX;
                    }
                    // Apply Row col information
                    m_omLctrSigList.vSetColumnInfo( nIndex,
                                                    def_RAW_VALUE_COLUMN,
                                                    sColType);
                    m_omLctrSigList.vSetNumericInfo( nIndex,
                                                     def_RAW_VALUE_COLUMN,
                                                     sNumInfo);
                    // Set the Raw Value
                    if( TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) )
                    {
                        // Mask unwanted portion of signal details
                        __int64 unVal= sSignalInfo.m_un64RawValue;
                        s_vRemoveUnwantedBits( unVal,sSignalInfo.m_ucSigLength );
                        omStrFormat.Format( defFORMAT_INT64_HEX, unVal);
                    }
                    else
                    {
                        if( psSignal->m_bySignalType == CHAR_INT)
                        {
                            omStrFormat.Format( defFORMAT_INT64_DECIMAL, sSignalInfo.m_un64RawValue);
                        }
                        else
                        {
                            omStrFormat.Format(defFORMAT_UINT64_DEC, sSignalInfo.m_un64RawValue);
                        }
                    }
                    // Set Raw Value
                    m_omLctrSigList.SetItemText( nIndex, def_RAW_VALUE_COLUMN,
                                                 omStrFormat);
                    // Set the unit
                    m_omLctrSigList.SetItemText( nIndex, def_SIG_UNIT_COLUMN,
                                                 sSignalInfo.m_omStrUnit);
                }
            }
            // Set the user type information for col 0.
            // This will show the signal information
            sColType.m_eType = eUser;
            m_omLctrSigList.vSetColumnInfo( nIndex,
                                            def_SIG_NAME_COLUMN,
                                            sColType );
            m_omLctrSigList.vSetUserProgInfo( nIndex,
                                              def_SIG_NAME_COLUMN,
                                              sProgInfo );
        }
        // Reset the data update flag
        m_bUpdating = FALSE;
    }
}

int CTxMsgDetailsView::nGetValidMessageID()
{
    UINT unMsgCode = nGetMessageID();
    // Check if it is valid.
    if(m_nRBTNFrameFormat == TRUE && unMsgCode >= defMAX_LMT_EXTD_MSG_ID)
    {
        unMsgCode = static_cast<UINT>(-1);
    }
    if(m_nRBTNFrameFormat == FALSE && unMsgCode >= MAX_LMT_FOR_STD_MSG_ID)
    {
        unMsgCode = static_cast<UINT>(-1);
    }
    return unMsgCode;
}

CWnd* CTxMsgDetailsView::pomGetParentWindow() const
{
    CWnd* pWnd = nullptr;
    // Get Splitter window pointer
    pWnd = GetParent();
    // Get Tx Msg Child Window pointer from Splitter window pointer
    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }
    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }

    if( pWnd != nullptr )
    {
        pWnd = pWnd->GetParent();
    }

    // Return Tx Msg Child window pointer or nullptr incase of failure
    return pWnd;
}

CWnd* CTxMsgDetailsView::pomGetBlocksViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgBlocksView );
    }
    // Return View Pointer
    return pView;
}

CWnd* CTxMsgDetailsView::pomGetListViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgMessageListView );
    }
    // Return View Pointer
    return pView;
}

CWnd* CTxMsgDetailsView::pomGetFunctionsViewPointer() const
{
    CWnd* pView = nullptr;
    // Get Child Frame Pointer
    CWnd* pWnd = nullptr;
    pWnd = pomGetParentWindow();
    // Get View Pointer
    if( pWnd != nullptr )
    {
        pView = ((CTxMsgChildFrame*)pWnd)->pomGetTxMsgViewPointers(
                    eTxMsgFunctionsView );
    }
    // Return View Pointer
    return pView;
}

BOOL CTxMsgDetailsView::bValidateData()
{
    // Assume Failure
    BOOL bIsValid = FALSE;
    // Selected Message Code
    int nMsgCode = -1;
    // User has to input atleast msg code and dlc
    CString omStr = "";
    UpdateData(TRUE);
    // Get the message ID.
    nMsgCode = nGetMessageID();
    // Check if it is valid.
    if(m_nRBTNFrameFormat == TRUE && nMsgCode >= defMAX_LMT_EXTD_MSG_ID )
    {
        nMsgCode = -1;
    }
    if(m_nRBTNFrameFormat == FALSE && nMsgCode >= MAX_LMT_FOR_STD_MSG_ID )
    {
        nMsgCode = -1;
    }
    if ( nMsgCode != -1 )
    {
        // Selected DLC Value
        USHORT usDLC = 0;
        m_odDLC.GetWindowText(omStr);
        if ( omStr.IsEmpty() != TRUE )
        {
            usDLC = (USHORT)m_odDLC.lGetValue();
            if(usDLC >= 0  && usDLC <=8)
            {
                bIsValid = TRUE;
            }
        }
        if(bIsValid == TRUE )
        {
            bIsValid = bCheckIfValueIsMoreThan255(usDLC);
            // The bCheckIfValueIsMoreThan255 function returns TRUE if
            // value is invalid
            bIsValid = !bIsValid;
        }
    }
    return bIsValid;
}

BOOL CTxMsgDetailsView::bCheckIfValueIsMoreThan255(USHORT usNoOfEditCtrlsToCheck)
{
    // Assume failure
    BOOL bResult = FALSE;
    // We have to check if the data byte value
    // in "Decimal" mode exceeds more than 255
    UINT unIDValue = IDC_EDIT_DB1;
    USHORT usTempCount = 0;

    CString omStrValue("");
    UINT unValue;
    CRadixEdit* pRadixEdit = nullptr;

    while ( ( usTempCount < usNoOfEditCtrlsToCheck ) &&
            ( bResult == FALSE ))
    {
        // Get value
        pRadixEdit = (CRadixEdit*)GetDlgItem(unIDValue);
        if ( pRadixEdit != nullptr)
        {
            unValue = static_cast<UINT> (pRadixEdit->lGetValue());
            if ( unValue > 255 )
            {
                CString omErrorStr;
                omErrorStr.Format( _(defSTR_INVALID_DATA_BYTES),
                                   usTempCount + 1 );
                bSetStatusText( omErrorStr );
                bResult = TRUE;// Exit loop
            }
            else
            {
                // Next control
                unIDValue++;// ID
                usTempCount++;// Count
            }
        }
    }
    return bResult;
}

void CTxMsgDetailsView::vPopulateMessageComboBox()
{
    m_omComboMsgIDorName.ResetContent();
    CMsgSignal* pDBptr =  m_pouDBPtr;
    // Get number of mesages in database
    if ( pDBptr != nullptr )
    {
        UINT unNoOfMessages = pDBptr->unGetNumerOfMessages();

        // Not zero
        if ( unNoOfMessages > 0 )
        {
            UINT* pIDArray = new UINT[unNoOfMessages];

            if (pIDArray != nullptr )
            {
                pDBptr->unListGetMessageIDs( pIDArray );
                sMESSAGE* pMessage = nullptr;
                // Add every message name into the message list
                for(UINT nCount=0 ; nCount<unNoOfMessages ; nCount++)
                {
                    pMessage = pDBptr->psGetMessagePointer( pIDArray[nCount] );
                    if(pMessage != nullptr)
                    {
                        CString omStrMsgName = pMessage->m_omStrMessageName;
                        CString omStrMsgId;
                        omStrMsgId.Format(defSTR_MSG_ID_IN_HEX,pMessage->m_unMessageCode);
                        omStrMsgName += omStrMsgId;

                        m_omComboMsgIDorName.AddString(omStrMsgName);
                    }
                }
                delete []pIDArray;
            }
        }
    }
    vAdjustWidthMessageComboBox();
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::vAdjustWidthMessageComboBox()
{
    CString str;
    CSize sz;
    int dx = 0;
    TEXTMETRIC tm;
    CDC* pDC = m_omComboMsgIDorName.GetDC();
    CFont* pFont = m_omComboMsgIDorName.GetFont();

    // Select the listbox font, save the old font
    CFont* pOldFont = pDC->SelectObject(pFont);
    // Get the text metrics for avg char width
    pDC->GetTextMetrics(&tm);

    for (int i = 0; i < m_omComboMsgIDorName.GetCount(); i++)
    {
        m_omComboMsgIDorName.GetLBText(i, str);
        sz = pDC->GetTextExtent(str);
        sz = pDC->GetTextExtent(str);

        // Add the avg width to prevent clipping
        sz.cx += tm.tmAveCharWidth;
        if (sz.cx > dx)
        {
            dx = sz.cx;
        }
    }

    // Select the old font back into the DC
    pDC->SelectObject(pOldFont);
    m_omComboMsgIDorName.ReleaseDC(pDC);
    // Adjust the width for the vertical scroll bar and the left and right border.
    dx += ::GetSystemMetrics(SM_CXVSCROLL) + 2*::GetSystemMetrics(SM_CXEDGE);

    // Set the width of the list box so that every item is completely visible.
    m_omComboMsgIDorName.SetDroppedWidth(dx);
}

void CTxMsgDetailsView::OnSelchangeCombMsgIdName()
{
    // Get Other view pointers
    CTxMsgBlocksView* pomBlockView = nullptr;
    CTxMsgListView* pomListView = nullptr;
    pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    if( pomBlockView != nullptr && pomListView != nullptr )
    {
        CString omStrMsgName = "";
        sMESSAGE* psMsg = nullptr;
        BOOL bValidMsgID = TRUE;

        int nSelectedIndex = m_omComboMsgIDorName.GetCurSel();
        if( nSelectedIndex != -1 )
        {
            m_omComboMsgIDorName.GetLBText( nSelectedIndex ,omStrMsgName );
            m_omComboMsgIDorName.SetWindowText( omStrMsgName );
        }
        // Get message Id from database in case user has selected a message name
        //if it is name [] is present
        int nIndex = omStrMsgName.Find(defMSGID_NAME_START_CHAR);
        if(nIndex != -1)
        {
            INT nMsgID = unGetMsgIDFromName(omStrMsgName);
            CMsgSignal* pDBptr =  m_pouDBPtr;
            if (nullptr != pDBptr)
            {
                // Check if it is a database message ID/Name
                psMsg = pDBptr->psGetMessagePointer(nMsgID);
            }

            if(psMsg == nullptr)
            {
                bValidMsgID = FALSE;
            }
            else
            {
                omStrMsgName.Format("%d", psMsg->m_unMessageLength);
                m_odDLC.SetWindowText( omStrMsgName);

                if(psMsg->m_bMessageFrameFormat == TRUE)
                {
                    CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_CHECKED);
                    CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_UNCHECKED);
                }
                else
                {
                    CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_UNCHECKED);
                    CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_CHECKED);
                }

                // Update Signal List
                STCAN_MSG sCanInfo;
                bUpdateMessageDetail(&sCanInfo);
                // Clear Status message
                bSetStatusText( "" );
                // Due to selection use the selected message name / ID
                // nGetMessage ID function uses GetWindowText and that will be
                // Updated only after the selection is done
                //sCanInfo.m_unMsgID = nMsgID; *** This is removed because of
                /* SetWindowText function call in case of selection */
                bUpdateSignalList(sCanInfo);
                // Update Signal Message Data Directly
                vUpdateSelectedMessageDetails();
                // Enable Add button
                // the vEnableAddButton uses GetWindowText.
                // So directly do processing here.
                PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
                // Get current block pointer
                psMsgCurrentBlock =
                    pomBlockView->psGetMsgBlockPointer(
                        pomBlockView->m_nSelectedMsgBlockIndex,
                        pomBlockView->m_psMsgBlockList );
                if( psMsgCurrentBlock != nullptr )
                {
                    // Check for the maximum count
                    if( psMsgCurrentBlock->m_unMsgCount < defMAX_MSGINBLOCK )
                    {
                        // Enable Add Button
                        m_omButtonUpdateMsg.EnableWindow();
                        // Enable Apply Button
                        CTxFunctionsView* pView =
                            ( CTxFunctionsView* )pomGetFunctionsViewPointer();
                        if( pView != nullptr )
                        {
                            if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
                            {
                                pView->m_omButtonApply.EnableWindow( TRUE );
                            }
                        }
                    }
                }

            }

            int             nCurrentIndex = -1;
            nCurrentIndex = pomListView ->m_omLctrMsgList.GetNextItem(nCurrentIndex,
                            LVNI_SELECTED );
            if(nCurrentIndex != -1)
            {
                vCallApplyChanges();
            }
        }
        else
        {
            bValidMsgID = FALSE;
            // Disable Signal Matrix
            m_odSignalMatrix.vSetMessageLength(0);
            // Remove All signals from the lsit
            m_omLctrSigList.DeleteAllItems();
            // Disable it
            m_omLctrSigList.EnableWindow( FALSE );
        }
    }
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::OnUpdateEditDLC()
{
    CWnd* pomWnd = GetFocus();
    // Avoid processing the data if the dialog is canceled
    if( pomWnd != nullptr)
    {
        INT unID = pomWnd->GetDlgCtrlID();
        // If the update is because of user change
        if( unID == IDC_EDIT_DLC )
        {
            BOOL bEntryValid = FALSE;
            // Get the message IS
            int nMsgID = nGetValidMessageID();
            // IF it is valid
            if( nMsgID != -1)
            {
                // Get DLC value
                int nDLC =  (int)(m_odDLC.lGetValue());
                // If DLC Text is empty the above function call will return
                // 0. So make it -1 to denote invalid value
                if( nDLC == 0 )
                {
                    CString omStrText;
                    m_odDLC.GetWindowText(omStrText);
                    if( omStrText.IsEmpty() == TRUE )
                    {
                        nDLC = -1;
                    }
                }
                // If it is valid
                if( nDLC >= 0 && nDLC < 9 )
                {
                    STCAN_MSG sCanInfo;
                    // Update Message details from UI
                    bUpdateMessageDetail(&sCanInfo);
                    // Update Signal List information
                    bUpdateSignalList(sCanInfo);
                    // Set the dirty flag to true
                    SetDirty();
                    // Update global message details
                    bEntryValid = vUpdateSelectedMessageDetails();
                }
                else
                {
                    // Show Invalid DLC text
                    bSetStatusText(_(defSTR_INVALID_DLC));
                }
            }
            // Enable/Disable Add Button
            if( IsWindow(m_omButtonUpdateMsg.m_hWnd) )
            {
                vEnableAddButton( bEntryValid );
            }
            // Clear Error Text
            if( bEntryValid )
            {
                bSetStatusText("");
            }
        }
    }
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::SetDirty(BOOL bDirty /* = TRUE */)
{
    int nItems = m_omLctrSigList.GetItemCount();
    int nImageIndex = bDirty == TRUE ? 2 : 0;
    LVITEM sItem;
    sItem.mask      = LVIF_IMAGE;
    sItem.iSubItem  = defMAIN_ITEM;
    for( int index = 0; index < nItems; index++)
    {
        sItem.iItem     = index;
        sItem.iImage    = nImageIndex;
        m_omLctrSigList.SetItem(&sItem);
    }
    m_bIsMsgDirty = bDirty;
}

void CTxMsgDetailsView::OnUpdateEditDataBytes()
{
    CWnd* pomWnd = GetFocus();
    // Avoid processing the data if the dialog is canceled
    if( pomWnd != nullptr)
    {
        INT unID = pomWnd->GetDlgCtrlID();
        // If the update is because of user change
        if( unID >= IDC_EDIT_DB1 && unID <= IDC_EDIT_DB8 )
        {
            BOOL bEntryValid = FALSE;
            // Get the message IS
            int nMsgID = nGetValidMessageID();
            // IF it is valid
            if( nMsgID != -1)
            {
                // Get DLC value
                int nDLC =  (int)(m_odDLC.lGetValue());
                // If DLC Text is empty the above function call will return
                // 0. So make it -1 to denote invalid value
                if( nDLC == 0 )
                {
                    CString omStrText;
                    m_odDLC.GetWindowText(omStrText);
                    if( omStrText.IsEmpty() == TRUE )
                    {
                        nDLC = -1;
                    }
                }
                // If it is valid
                if( nDLC >= 0 && nDLC < 9 )
                {
                    STCAN_MSG sCanInfo;
                    // Update Message details from UI
                    bUpdateMessageDetail(&sCanInfo);
                    // Update Signal List information
                    bUpdateSignalList(sCanInfo);
                    // Set the dirty flag to true
                    SetDirty();
                    // Update global message details
                    bEntryValid = vUpdateSelectedMessageDetails();
                    vCallApplyChanges(); //call this function to update data in case of AutoUpdate
                }
            }

            // Enable/Disable Add Button
            if( IsWindow(m_omButtonUpdateMsg.m_hWnd) )
            {
                vEnableAddButton( bEntryValid );
            }
            // Clear Error Text
            if( bEntryValid )
            {
                bSetStatusText("");
            }
        }
		else if ( (unID >= IDC_EDIT_ROLL_STARTBIT && unID <= IDC_EDIT_CHECKSUM_BYTE))
		{
			bUpdateMessageRollCntCheckSum();
		}
		
    }
}

void CTxMsgDetailsView::OnRbtnMsgtypeStd()
{
    BOOL bEntryValid = FALSE;
    UpdateData(TRUE);
    int nMsgID = nGetValidMessageID();
    if( nMsgID != -1)
    {
        // Get DLC value
        int nDLC =  (int)(m_odDLC.lGetValue());
        // If DLC Text is empty the above function call will return
        // 0. So make it -1 to denote invalid value
        if( nDLC == 0 )
        {
            CString omStrText;
            m_odDLC.GetWindowText(omStrText);
            if( omStrText.IsEmpty() == TRUE )
            {
                nDLC = -1;
            }
        }
        if( nDLC >= 0 && nDLC < 9 )
        {
            STCAN_MSG sCanInfo;
            bUpdateMessageDetail(&sCanInfo);
            bUpdateSignalList(sCanInfo);
            vUpdateSelectedMessageDetails();
            bEntryValid = TRUE;
        }
        else
        {
            bSetStatusText(_(defSTR_INVALID_DLC));
        }

        vCallApplyChanges();
    }
    else
    {
        bSetStatusText(_(defSTR_INVALID_MESSAGE_ID));
    }

    // Enable/Disable Add Button
    if( IsWindow(m_omButtonUpdateMsg.m_hWnd) )
    {
        vEnableAddButton(bEntryValid);
    }
    if( bEntryValid )
    {
        bSetStatusText("");
    }
}

void CTxMsgDetailsView::OnChkbMsgtypeRtr()
{
    // RTR is changed
    // If enabled then disable signal details and Signal Matrix
    UpdateData(TRUE);
    if(m_bIsRTR == TRUE )
    {
        // Remove Signal Informations
        // Clear Signal List
        m_omLctrSigList.DeleteAllItems();
        // Disable Signal List
        m_omLctrSigList.EnableWindow(FALSE);

        if( m_odSignalMatrix.m_hWnd != nullptr )
        {
            // Reset the values
            m_odSignalMatrix.vResetValues();
            // Set the message Length
            m_odSignalMatrix.vSetMessageLength(0);
        }
    }
    else
    {
        STCAN_MSG sCanInfo;
        bUpdateMessageDetail(&sCanInfo);
        INT nMsgCode = nGetValidMessageID();
        if ( nMsgCode == -1 )       //AUC, signal matrix gets enabled for invalid signal
        {
            sCanInfo.m_ucDataLen = 0;
        }
        bUpdateSignalList(sCanInfo);
        m_odSignalMatrix.vSetMessageLength(sCanInfo.m_ucDataLen);
    }
    // Now Update the message data
    BOOL bEntryValid = FALSE;
    INT nMsgCode = nGetValidMessageID();
    // Check if it is valid.
    if ( nMsgCode != -1 )
    {
        // Get DLC value
        int nDLC =  (int)(m_odDLC.lGetValue());
        // If DLC Text is empty the above function call will return
        // 0. So make it -1 to denote invalid value
        if( nDLC == 0 )
        {
            CString omStrText;
            m_odDLC.GetWindowText(omStrText);
            if( omStrText.IsEmpty() == TRUE )
            {
                nDLC = -1;
            }
        }
        if( nDLC >= 0 && nDLC < 9 )
        {
            STCAN_MSG sCanInfo;
            bUpdateMessageDetail(&sCanInfo);
            bUpdateSignalList(sCanInfo);
            vUpdateSelectedMessageDetails();
            bEntryValid = TRUE;
        }
        else
        {
            bSetStatusText(_(defSTR_INVALID_DLC));
        }
        vCallApplyChanges();
    }
    else
    {
        bSetStatusText(_(defSTR_INVALID_MESSAGE_ID));
    }

    // Enable/Disable Add Button
    if( IsWindow(m_omButtonUpdateMsg.m_hWnd) )
    {
        vEnableAddButton( bEntryValid );
    }
    if( bEntryValid )
    {
        bSetStatusText( "" );
    }
}

void CTxMsgDetailsView::OnButtonAddMsg()
{
    BOOL bDataValid = FALSE;
    // Validate the data
    bDataValid = bValidateData();
    if(bDataValid == TRUE )
    {
        // Add the message block
        bAddMsgInBlock();
        // Apply the changes
        // Don't apply changes not
        //vApplyChanges();
        // Enable Apply Button
        CTxFunctionsView* pView =
            ( CTxFunctionsView* )pomGetFunctionsViewPointer();
        if( pView != nullptr )
        {
            if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_UNCHECKED)
            {
                pView->m_omButtonApply.EnableWindow( TRUE );
            }
        }
        //Update the global message store
        vCallApplyChanges();
        if(CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG))
        {
            PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
            CTxMsgBlocksView* pomBlockView = nullptr;
            pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
            psMsgCurrentBlock =  pomBlockView->psGetMsgBlockPointer(
                                     pomBlockView->m_nSelectedMsgBlockIndex,
                                     pomBlockView->m_psMsgBlockList );
            if((psMsgCurrentBlock->m_unMsgCount == 1)&& !CTxMsgManager::s_bDelayBetweenBlocksOnly)      //jus now a msg was added
            {
                //if delay btwn msg block is enabled den doont create a new thread
                CTxMsgManager::s_podGetTxMsgManager()->vStartTransmission(0);
            }
        }
    }
    vUpdateStateDataBytes();
}

BOOL CTxMsgDetailsView::bAddMsgInBlock()
{
    BOOL bReturn = TRUE;
    // Get Other view pointers
    CTxMsgBlocksView* pomBlockView = nullptr;
    CTxMsgListView* pomListView = nullptr;
    pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    if( pomBlockView != nullptr && pomListView != nullptr )
    {
        bReturn = TRUE;
        PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
        psMsgCurrentBlock =  pomBlockView->psGetMsgBlockPointer(
                                 pomBlockView->m_nSelectedMsgBlockIndex,
                                 pomBlockView->m_psMsgBlockList );
        if(psMsgCurrentBlock != nullptr )
        {
            PSTXCANMSGLIST psTxMsgList = nullptr;
            PSTXCANMSGLIST psTxCurrentMsgList = nullptr;
            psTxMsgList = new STXCANMSGLIST;
            if(psTxMsgList != nullptr )
            {
                if(psMsgCurrentBlock->m_unMsgCount >0)
                {
                    psTxCurrentMsgList =  pomListView->psGetMsgDetailPointer(
                                              psMsgCurrentBlock->m_unMsgCount-1,
                                              psMsgCurrentBlock );
                    if(psTxCurrentMsgList != nullptr )
                    {
                        psTxCurrentMsgList->m_psNextMsgDetails = psTxMsgList;
                    }
                }
                else
                {
                    psMsgCurrentBlock->m_psTxCANMsgList = psTxMsgList;
                    //Enable Delete All for the first node insert only
                    // if transmission is off.
                    pomListView->m_omButtonDeleteAllMsg.EnableWindow(!CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_SENDMESG));
                }
                psTxMsgList->m_unIndex = psMsgCurrentBlock->m_unMsgCount;
                psTxMsgList->m_bModified = true;
                (psMsgCurrentBlock->m_unMsgCount)++;
                psTxMsgList->m_psNextMsgDetails = nullptr;
                psTxMsgList->m_sTxMsgDetails.m_bIsMsgDirty = m_bIsMsgDirty;
                if(psMsgCurrentBlock->m_unMsgCount >=  defMAX_MSGINBLOCK )
                {
                    m_omButtonUpdateMsg.EnableWindow(FALSE);
                }
                bReturn =
                    bUpdateMessageDetail( &(psTxMsgList->
                                            m_sTxMsgDetails.m_sTxMsg) );
                if(bReturn == TRUE )
                {
                    // Set Enabled Flag to True
                    psTxMsgList->m_sTxMsgDetails.m_bEnabled = TRUE;
                    pomListView->vUpdateMsgListDisplay(
                        psTxMsgList->m_sTxMsgDetails,
                        -1 );
                }
            }
            else
            {
                bReturn = FALSE;
            }
        }
    }
    vUpdateStateDataBytes();

    return bReturn;
}

void CTxMsgDetailsView::OnItemchangedLstcSigDetails( NMHDR* pNMHDR,
        LRESULT* pResult )
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    // Update Matrix while selection got changed or updating
    // through program
    if(pNMListView->uChanged != LVIF_TEXT )
    {
        // Selected & Focused
        if( pNMListView->uNewState == (LVIS_FOCUSED | LVIS_SELECTED) )
        {
            // For Row selection
            vUpdateSignalMatrix( pNMListView->iItem);
        }
    }
    // Notification for text change
    if( pNMListView->uChanged == LVIF_TEXT && m_bUpdating == FALSE)
    {
        LV_DISPINFO ls;
        // Get the change information from the list control
        if(m_omLctrSigList.sGetModificationStructure(ls) == TRUE)
        {
            switch( ls.item.iSubItem)
            {
                    // Raw Value Change
                case 1:
                    vUpdateFromRawValue( ls.item.iItem,
                                         ls.item.iSubItem );
                    break;
                    // Physical Value Change
                case 2:
                    vUpdateFromPhysicalValue( ls.item.iItem,
                                              ls.item.iSubItem );
                    break;
            }
        }
        vCallApplyChanges();
    }

    *pResult = 0;
}

void CTxMsgDetailsView::vUpdateAllBlocksFrmDB()
{
    CMsgSignal* pDBptr = nullptr;
    CTxMsgBlocksView* pomBlockView = nullptr;
    PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;

    pomBlockView = ( CTxMsgBlocksView* )pomGetBlocksViewPointer();
    pDBptr =  m_pouDBPtr;
    for(int nMsgCnt =0; nMsgCnt < pDBptr->unGetNumerOfMessages(); nMsgCnt++)
    {
        psMsgCurrentBlock =  pomBlockView->m_psMsgBlockList ;
        while(psMsgCurrentBlock != nullptr)
        {
            PSTXCANMSGLIST psTxMsgList = nullptr;
            psTxMsgList = psMsgCurrentBlock->m_psTxCANMsgList;
            while(psTxMsgList != nullptr)
            {
                if(psMsgCurrentBlock->m_unMsgCount >0)
                {
                    UINT    unMsgID = psTxMsgList->m_sTxMsgDetails.m_sTxMsg.m_unMsgID;
                    sMESSAGE*  psMsg = pDBptr->psGetMessagePointer(unMsgID);
                    if(psMsg != nullptr)
                    {
                        CString csMsgLen = pDBptr->omStrGetMessageLengthFromMsgCode(unMsgID);
                        psTxMsgList->m_sTxMsgDetails.m_sTxMsg.m_ucDataLen =  atoi(csMsgLen);
                        psMsgCurrentBlock->m_bModified = true;
                    }
                }
                psTxMsgList = psTxMsgList->m_psNextMsgDetails;
            }
            psMsgCurrentBlock = psMsgCurrentBlock->m_psNextMsgBlocksList ;
        }
    }
}

void CTxMsgDetailsView::vUpdateSignalMatrix(int nSelectedIndex)
{
    sSIGNALS* psSignal = nullptr;
    // Get the signal pointer
    psSignal = psGetSelectedSignalStruct(nSelectedIndex);
    // If it is a valid signal
    if( psSignal != nullptr )
    {
        // Calculate the Signal mask
        BYTE abySigMask[DATA_LENGTH_MAX] = {0};
        CMsgSignal::bCalcBitMaskForSig( abySigMask,
                                        DATA_LENGTH_MAX,
                                        psSignal->m_unStartByte,
                                        psSignal->m_byStartBit,
                                        psSignal->m_unSignalLength,
                                        psSignal->m_eFormat);
        // Update the matrix
        UINT nMsgLen = 0;
        if (m_psSelectedMsgDetails != nullptr)
        {
            nMsgLen = m_psSelectedMsgDetails->m_unMessageLength;
        }
        m_odSignalMatrix.vSetHighlight(abySigMask, nMsgLen);//unFrom, unLength);
    }
}

void CTxMsgDetailsView::vUpdateFromRawValue(int nItem, int nSubItem)
{
    CString omStrSignalName;
    sSIGNALS* psSignal = nullptr;
    BOOL bDataInvalid = FALSE;

    psSignal = psGetSelectedSignalStruct(nItem);
    // If it is a valid signal
    if( psSignal != nullptr )
    {
        // Get the Signal Raw Value
        CString omStr = m_omLctrSigList.GetItemText( nItem, nSubItem );
        CString omstrDefault;
        __int64 nI64SignVal;
        // Set the base value
        int nBase = CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) ? defBASE_HEX : defBASE_DEC;
        // Get the int 64 val from the string
        bConvertStringToInt64( omStr, nI64SignVal, nBase );
        // Extending the sign bit is required for hex only
        // For decimal mode it is not required
        if( psSignal->m_bySignalType == CHAR_INT )
        {
            s_vExtendSignBit( nI64SignVal,psSignal->m_unSignalLength );
        }
        // If the mode is in Dec then change the min and max value
        if( psSignal->m_bySignalType == CHAR_INT )
        {
            __int64 n64Min, n64Max;
            n64Min = psSignal->m_SignalMinValue.n64Value;
            n64Max = psSignal->m_SignalMaxValue.n64Value;
            s_vExtendSignBit( n64Min,psSignal->m_unSignalLength );
            s_vExtendSignBit( n64Max,psSignal->m_unSignalLength );

            if ( nI64SignVal > n64Max )
            {
                nI64SignVal = n64Max;
                bDataInvalid = TRUE;
            }
            else if ( nI64SignVal < n64Min )
            {
                nI64SignVal = n64Min;
                bDataInvalid = TRUE;
            }
        }
        else
        {
            if ( (UINT64)nI64SignVal > psSignal->m_SignalMaxValue.un64Value )
            {
                nI64SignVal = psSignal->m_SignalMaxValue.un64Value;
                bDataInvalid = TRUE;
            }
            else if ( (UINT64)nI64SignVal < psSignal->m_SignalMinValue.un64Value )
            {
                nI64SignVal = psSignal->m_SignalMinValue.un64Value;
                bDataInvalid = TRUE;
            }
        }

        // Update the data
        UCHAR ucData[defMAX_BYTE] = {0};
        //copy the existing data
        memcpy(ucData, m_unData, sizeof(BYTE) * defMAX_BYTE);
        //update the data
        sSIGNALS::vSetSignalValue(psSignal, ucData, nI64SignVal);
        //copy the data
        memcpy(m_unData, ucData, sizeof(UCHAR) * defMAX_BYTE);
        //commented by kadoor vUpdateSignalData( psSignal, nI64SignVal );
        vUpdateDataBytes();
        BOOL bFound = FALSE;
        // Set the Raw Value
        if( TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) )
        {
            // For signed value
            if( psSignal->m_bySignalType == CHAR_INT )
            {
                // Mask unwanted portion of signal details
                __int64 unVal = nI64SignVal;
                s_vRemoveUnwantedBits(unVal,psSignal->m_unSignalLength);
                omstrDefault.Format( defFORMAT_INT64_HEX, unVal);
            }
            // For unsigned values
            else
            {
                omstrDefault.Format( defFORMAT_INT64_HEX, nI64SignVal);
            }
        }
        // For formatting in decimal
        else
        {
            if( psSignal->m_bySignalType == CHAR_INT )
            {
                omstrDefault.Format( defFORMAT_INT64_DECIMAL, nI64SignVal);
            }
            else
            {
                omstrDefault.Format(defFORMAT_UINT64_DEC, nI64SignVal);
            }
        }

        m_omLctrSigList.SetItemText( nItem, def_RAW_VALUE_COLUMN,
                                     omstrDefault );
        // Set the physical Value
        // Check for Signal Descriptor
        if( psSignal->m_oSignalIDVal != nullptr )
        {
            CSignalDescVal* psDesc = nullptr;
            psDesc = psSignal->m_oSignalIDVal;
            while( psDesc != nullptr && bFound == FALSE )
            {
                //                if( psDesc->m_n64SignalVal == nI64SignVal )
                if( psDesc->m_DescValue.n64Value == nI64SignVal )
                {
                    omstrDefault = psDesc->m_omStrSignalDescriptor;
                    bFound = TRUE;
                }
                else
                {
                    psDesc = psDesc->m_pouNextSignalSignalDescVal;
                }
            }
        }
        // If signal descriptor is not found take the value entered
        if( bFound == FALSE )
        {
            BOOL bFloat = FALSE;
            // Check for possible float value
            // Check Factor. Float Value - Int value should be greater
            // then 0 to have floating point value
            if( ( psSignal->m_fSignalFactor -
                    static_cast<int>(psSignal->m_fSignalFactor) ) > 0.0 )
            {
                bFloat = TRUE;
            }
            // Check Offset only if the float flag is not set
            else if( psSignal->m_fSignalOffset -
                     static_cast<int>(psSignal->m_fSignalOffset) > 0.0 )
            {
                bFloat = TRUE;
            }
            if( bFloat == TRUE )
            {
                // Calculate Physical Value and update
                double dPhyValue = nI64SignVal * psSignal->m_fSignalFactor +
                                   psSignal->m_fSignalOffset;
                omstrDefault.Format(defSTR_FORMAT_PHY_VALUE, dPhyValue);
            }
            else
            {
                nI64SignVal = __int64(nI64SignVal * psSignal->m_fSignalFactor +
                                      psSignal->m_fSignalOffset);
                if( psSignal->m_bySignalType == CHAR_INT )
                {
                    omstrDefault.Format(defFORMAT_INT64_DECIMAL, nI64SignVal);
                }
                else
                {
                    omstrDefault.Format(defFORMAT_UINT64_DEC, nI64SignVal);
                }
            }
        }

        m_omLctrSigList.SetItemText( nItem, def_PHY_VALUE_COLUMN,
                                     omstrDefault );

        // Set the data value
        vUpdateSignalMatrix();
        //m_odSignalMatrix.vSetValue( m_bData );
        SetDirty(FALSE);
        vUpdateSelectedMessageDetails();
    }
}

void CTxMsgDetailsView::vUpdateFromPhysicalValue(int nItem, int nSubItem)
{
    CString omStrSignalName;
    sSIGNALS* psSignal = nullptr;

    psSignal = psGetSelectedSignalStruct(nItem);
    // If it is a valid signal
    if( psSignal != nullptr )
    {
        CString omstrDefault;
        double dSignVal = 0.0;
        __int64 n64SigVal = 0;
        UINT64 un64SigVal = 0;
        // Get the string from the list item
        CString omStr = m_omLctrSigList.GetItemText( nItem, nSubItem );
        BOOL bFound = FALSE;
        BOOL bDataInvalid = FALSE;
        // Check for Signal Descriptor
        if( psSignal->m_oSignalIDVal != nullptr )
        {
            CSignalDescVal* psDesc = nullptr;
            psDesc = psSignal->m_oSignalIDVal;

            while( psDesc != nullptr && bFound == FALSE )
            {
                if( psDesc->m_omStrSignalDescriptor.Compare(omStr) == 0 )
                {
                    //                    n64SigVal = psDesc->m_n64SignalVal;
                    n64SigVal = psDesc->m_DescValue.n64Value;
                    un64SigVal = psDesc->m_DescValue.un64Value;
                    bFound = TRUE;
                }
                else
                {
                    psDesc = psDesc->m_pouNextSignalSignalDescVal;
                }
            }
        }
        // Proceed for Non Signal Descriptor values and Signals with out any
        // Signal Descriptor
        if( bFound == FALSE)
        {
            // Calculate Raw Value
            // Get the Signal Raw Value

            // Get the value
            dSignVal = 0;

            dSignVal = atof(omStr);


            if(psSignal->m_bySignalType == CHAR_INT)
            {
                /*n64SigVal = static_cast<UINT64>(
                                ( dSignVal - psSignal->m_fSignalOffset) /
                                psSignal->m_fSignalFactor );*/

                // To handle the float data type precision values which is not zero
                // (For Ex: 0.3000 is displayed as 0.300000001)
                float f64SigVal = ( dSignVal - psSignal->m_fSignalOffset) /
                                  psSignal->m_fSignalFactor;
                n64SigVal = static_cast<INT64>(f64SigVal);
                if (f64SigVal - (INT64)(f64SigVal) > 0.99990000)
                {
                    n64SigVal = static_cast<INT64>(ceil(f64SigVal));
                }
                else if (f64SigVal - (INT64)(f64SigVal) < 0.99990000)
                {
                    n64SigVal = static_cast<INT64>(floor(f64SigVal));
                }
            }
            else
            {
                // Apply factor & offset
                /*un64SigVal = static_cast<UINT64>(
                                 ( dSignVal - psSignal->m_fSignalOffset) /
                                 psSignal->m_fSignalFactor );*/

                // To handle the float data type precision values which is not zero
                // (For Ex: 0.3000 is displayed as 0.300000001)
                float f64SigVal = ( dSignVal - psSignal->m_fSignalOffset) /
                                  psSignal->m_fSignalFactor;
                un64SigVal = static_cast<UINT64>(f64SigVal);
                if (f64SigVal - (UINT64)(f64SigVal) > 0.99990000)
                {
                    un64SigVal = static_cast<UINT64>(ceil(f64SigVal));
                }
            }
            // Check for Min-Max Limit
            // If signed extend the sign bit
            if(psSignal->m_bySignalType == CHAR_INT)
            {
                __int64 nVal = psSignal->m_SignalMaxValue.n64Value;
                if( psSignal->m_bySignalType == CHAR_INT )
                {
                    s_vExtendSignBit( nVal,psSignal->m_unSignalLength );
                }

                if ( n64SigVal > nVal )
                {
                    n64SigVal = nVal;
                    bDataInvalid = TRUE;
                }
                else
                {
                    nVal = psSignal->m_SignalMinValue.n64Value;
                    if( psSignal->m_bySignalType == CHAR_INT )
                    {
                        s_vExtendSignBit( nVal,
                                          psSignal->m_unSignalLength );
                    }

                    if ( n64SigVal < nVal )
                    {
                        n64SigVal = nVal;
                        bDataInvalid = TRUE;
                    }
                }
            }
            else
            {
                UINT64 nVal = psSignal->m_SignalMaxValue.un64Value;
                /*if( psSignal->m_bySignalType == CHAR_INT )
                {
                s_vExtendSignBit( nVal,psSignal->m_unSignalLength );
                }*/

                if ( un64SigVal > nVal )
                {
                    un64SigVal = nVal;
                    bDataInvalid = TRUE;
                }
                else
                {
                    nVal = psSignal->m_SignalMinValue.un64Value;
                    if ( un64SigVal < nVal )
                    {
                        un64SigVal = nVal;
                        bDataInvalid = TRUE;
                    }
                }
            }
        }
        // Update the data
        UCHAR ucData[8] = {0};
        //copy the existing data
        memcpy(ucData, m_unData, sizeof(BYTE) * defMAX_BYTE);
        //update the data
        if(psSignal->m_bySignalType == CHAR_INT)
        {
            sSIGNALS::vSetSignalValue(psSignal, ucData, n64SigVal);
        }
        else
        {
            sSIGNALS::vSetSignalValue(psSignal, ucData, /*n64SigVal*/un64SigVal);
        }

        //copy to the memeber variable
        memcpy(m_unData, ucData, sizeof(BYTE) * defMAX_BYTE);

        //commented by kadoor vUpdateSignalData( psSignal, n64SigVal );
        vUpdateDataBytes();
        // Calculate Physical Value and update
        // Set the Raw Value
        if( TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX))
        {
            if( psSignal->m_bySignalType == CHAR_INT )
            {
                __int64 un64Val = n64SigVal;
                s_vRemoveUnwantedBits( un64Val, psSignal->m_unSignalLength);
                omstrDefault.Format( defFORMAT_INT64_HEX, un64Val);
            }
            else
            {
                UINT64 un64Val = un64SigVal;
                s_vRemoveUnwantedBits( un64Val, psSignal->m_unSignalLength);
                omstrDefault.Format( defFORMAT_INT64_HEX, un64Val);
            }
        }
        else
        {
            if( psSignal->m_bySignalType == CHAR_INT )
            {
                omstrDefault.Format( defFORMAT_INT64_DECIMAL, n64SigVal);
            }
            else
            {
                omstrDefault.Format(defFORMAT_UINT64_DEC, un64SigVal);
            }
        }

        m_omLctrSigList.SetItemText( nItem, def_RAW_VALUE_COLUMN,
                                     omstrDefault );
        // Set back the phy value. This will be the actual phy with
        // decimal loss
        // Check for Signal Descriptor value
        if ( bFound == FALSE || bDataInvalid == TRUE)
        {
            if( psSignal->m_oSignalIDVal != nullptr )
            {
                CSignalDescVal* psDesc = nullptr;
                psDesc = psSignal->m_oSignalIDVal;

                while( psDesc != nullptr && bFound == FALSE )
                {
                    //                    if( psDesc->m_n64SignalVal == n64SigVal )
                    if( psDesc->m_DescValue.n64Value == n64SigVal )
                    {
                        omstrDefault = psDesc->m_omStrSignalDescriptor;
                        bFound = TRUE;
                    }
                    else
                    {
                        psDesc = psDesc->m_pouNextSignalSignalDescVal;
                    }
                }
            }
            if( bFound == FALSE )
            {
                if( psSignal->m_bySignalType == CHAR_INT )
                {
                    s_vExtendSignBit( n64SigVal,psSignal->m_unSignalLength );
                }
                BOOL bFloat = FALSE;
                // Check for possible float value
                // Check Factor. Float Value - Int value should be greater
                // then 0 to have floating point value
                if( ( psSignal->m_fSignalFactor -
                        static_cast<int>(psSignal->m_fSignalFactor) ) > 0.0 )
                {
                    bFloat = TRUE;
                }
                // Check Offset only if the float flag is not set
                else if( psSignal->m_fSignalOffset -
                         static_cast<int>(psSignal->m_fSignalOffset) > 0.0 )
                {
                    bFloat = TRUE;
                }
                if( bFloat == TRUE )
                {
                    if( psSignal->m_bySignalType == CHAR_INT )
                    {
                        dSignVal = n64SigVal * psSignal->m_fSignalFactor +
                                   psSignal->m_fSignalOffset;
                        omstrDefault.Format(defSTR_FORMAT_PHY_VALUE, dSignVal);
                    }
                    else
                    {
                        dSignVal = un64SigVal * psSignal->m_fSignalFactor +
                                   psSignal->m_fSignalOffset;
                        omstrDefault.Format(defSTR_FORMAT_PHY_VALUE, dSignVal);
                    }
                }
                else
                {

                    if( psSignal->m_bySignalType == CHAR_INT )
                    {
                        n64SigVal = __int64(n64SigVal * psSignal->m_fSignalFactor +
                                            psSignal->m_fSignalOffset);
                        omstrDefault.Format(defFORMAT_INT64_DECIMAL, n64SigVal);
                    }
                    else
                    {
                        un64SigVal = UINT64(un64SigVal * psSignal->m_fSignalFactor +
                                            psSignal->m_fSignalOffset);
                        omstrDefault.Format(defFORMAT_UINT64_DEC, un64SigVal);
                    }
                }

            }
            m_omLctrSigList.SetItemText( nItem, def_PHY_VALUE_COLUMN,
                                         omstrDefault );
        }
        // Set the data value
        vUpdateSignalMatrix();
        //m_odSignalMatrix.vSetValue( m_bData );
        SetDirty(FALSE);
        vUpdateSelectedMessageDetails();
    }
}

void CTxMsgDetailsView::vUpdateSignalData(sSIGNALS* psSignal,
        __int64 nI64SignVal )
{
    if( psSignal != nullptr )
    {
        __int64 unSignalMask  = 1;

        int nBitIndex = ( psSignal->m_unStartByte  - 1 ) *
                        defBITS_IN_BYTE +
                        psSignal->m_byStartBit;
        int nLength = psSignal->m_unSignalLength;

        while ( nLength)
        {
            if ( nI64SignVal & unSignalMask )
            {
                m_bData[nBitIndex] = 1;
            }
            else
            {
                m_bData[nBitIndex] = 0;
            }

            unSignalMask <<= 1;
            nBitIndex++;
            nLength--;
        }

        for ( UINT unIndex = 0;
                unIndex < m_psSelectedMsgDetails->m_unMessageLength;
                unIndex++ )
        {
            int nArrIndex = unIndex * 8;

            m_unData[unIndex].sByte.Bit0 = m_bData[nArrIndex];
            m_unData[unIndex].sByte.Bit1 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit2 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit3 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit4 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit5 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit6 = m_bData[++nArrIndex];
            m_unData[unIndex].sByte.Bit7 = m_bData[++nArrIndex];
        }
    }
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::vUpdateDataBytes()
{
    // Display databytes
    UINT unIDValue = IDC_EDIT_DB1;// ID of first databyte edit control
    CEdit* pEdit = nullptr;
    CString omStr, omStrFormatData;

    omStrFormatData = (CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX) == TRUE) ? defFORMAT_DATA_HEX : defFORMAT_DATA_DECIMAL;

    for(UINT i =0 ; i< defMAX_BYTE; i++ )
    {
        pEdit = (CEdit*)GetDlgItem(unIDValue + i);
        int index = ( m_psSelectedMsgDetails->m_nMsgDataFormat != DATA_FORMAT_INTEL)?
                    m_psSelectedMsgDetails->m_unMessageLength - i - 1: i;
        if ( pEdit != nullptr  )
        {
            if(i >= m_psSelectedMsgDetails->m_unMessageLength)
            {
                omStr = "";
            }
            else
            {
                omStr.Format(omStrFormatData ,m_unData[index].byByte);
            }
            pEdit->SetWindowText(omStr);
        }
    }
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::vUpdateStateDataBytes()
{
    unsigned int dlc = (unsigned int) m_odDLC.lGetValue();

    m_odDB1.SetReadOnly(dlc < 1);
    m_odDB2.SetReadOnly(dlc < 2);
    m_odDB3.SetReadOnly(dlc < 3);
    m_odDB4.SetReadOnly(dlc < 4);
    m_odDB5.SetReadOnly(dlc < 5);
    m_odDB6.SetReadOnly(dlc < 6);
    m_odDB7.SetReadOnly(dlc < 7);
    m_odDB8.SetReadOnly(dlc < 8);
}

void CTxMsgDetailsView::vSetValues(PSTXCANMSGLIST psTxMsgList)
{
    CString omStr            = "";
    CString omFormat         = "";
    CString omStrFormatData  = "";

	STXCANMSGDETAILS* psTxMsg = &(psTxMsgList->m_sTxMsgDetails);
    // If the pointer is null
    // then clear the contents of all the undefined msg controls
    if ( psTxMsgList != nullptr)
    {
        if( TRUE == CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX))
        {
            omFormat        = defFORMAT_MSGID_HEX_STR;
            omStrFormatData = defFORMAT_DATA_HEX;
        }
        else
        {
            omFormat        = defFORMAT_MSGID_DECIMAL;
            omStrFormatData = defFORMAT_DATA_DECIMAL;
        }
        UINT unMsgID = psTxMsg->m_sTxMsg.m_unMsgID;
        CString omStrMsgLength = "";
        // Get the message name from the database
        CMsgSignal* pDBptr =  m_pouDBPtr;
        if (nullptr != pDBptr)
        {
            omStr =  pDBptr->omStrGetMessageNameFromMsgCode(unMsgID);
        }

        // If it is not a DB message then use the numeric value
        if( omStr.IsEmpty() == TRUE)
        {
            BOOL bisHex = CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX);
            if( !bisHex )
            {
                omStr.Format( defFORMAT_MSGID_DECIMAL, unMsgID);
            }
            else
            {
                omStr.Format( defFORMAT_MSGID_HEX_STR, unMsgID);
            }
        }
        else
        {
            //Add the message ID with the name
            CString omStrMsgId;
            omStrMsgId.Format(defSTR_MSG_ID_IN_HEX,unMsgID);
            omStr += omStrMsgId;
        }
        // Display code, DLC
        m_omComboMsgIDorName.SetWindowText(omStr);
        omStr.Format( defFORMAT_MSGID_DECIMAL, psTxMsg->m_sTxMsg.m_ucDataLen);
        m_odDLC.SetWindowText(omStr);
        // Display databytes
        UINT unIDValue = IDC_EDIT_DB1;// ID of first databyte edit control
        CEdit* pEdit = nullptr;

        for(INT i =0 ; i< defMAX_BYTE; i++ )
        {
            pEdit = (CEdit*)GetDlgItem(unIDValue + i);
            if ( pEdit != nullptr  )
            {
                if(i>= psTxMsg->m_sTxMsg.m_ucDataLen)
                {
                    omStr = "";
                }
                else
                {
                    omStr.Format(omStrFormatData,psTxMsg->m_sTxMsg.m_ucData[i]);
                }
                pEdit->SetWindowText(omStr);
            }
        }
		//if (m_enRollChk.GetCheck())
		{
			pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ROLL_STARTBIT);
			if (pEdit != nullptr)
			{
				omStr.Format(omStrFormatData, psTxMsgList->m_rollCntStartBit);
				pEdit->SetWindowText(omStr);
			}
			pEdit = (CEdit*)GetDlgItem(IDC_EDIT_ROLL_MAX);
			if (pEdit != nullptr)
			{
				omStr.Format(omStrFormatData, psTxMsgList->m_rollCntMax);
				pEdit->SetWindowText(omStr);
			}
			pEdit = (CEdit*)GetDlgItem(IDC_EDIT_CHECKSUM_BYTE);
			if (pEdit != nullptr)
			{
				omStr.Format(omStrFormatData, psTxMsgList->m_checkSumByte);
				pEdit->SetWindowText(omStr);
			}
			m_omCheckSumType.SetCurSel(psTxMsgList->m_checkSumType);
			m_enRollChk.SetCheck(psTxMsgList->m_bEnRollCntCheck);
		}
		

		
		
        // check the message type selected.
        if(psTxMsg->m_sTxMsg.m_ucRTR == TRUE)
        {
            CheckDlgButton(IDC_CHKB_MSGTYPE_RTR,BST_CHECKED);
        }
        else
        {
            CheckDlgButton(IDC_CHKB_MSGTYPE_RTR,BST_UNCHECKED);
        }
        if(psTxMsg->m_sTxMsg.m_ucEXTENDED == TRUE)
        {
            CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_CHECKED);
            CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_UNCHECKED);
        }
        else if(psTxMsg->m_sTxMsg.m_ucEXTENDED == FALSE)
        {
            CheckDlgButton(IDC_RBTN_MSGTYPE_EXTD,BST_UNCHECKED);
            CheckDlgButton(IDC_RBTN_MSGTYPE_STD,BST_CHECKED);
        }
        // Update Channel ID
        int nChannelIDIndex = psTxMsg->m_sTxMsg.m_ucChannel - 1;
        // Check for valid value. If it is invalid set the selection to the
        // default channel
        if( nChannelIDIndex < 0 )
        {
            nChannelIDIndex = 0;
        }
        m_omComboChannelID.SetCurSel( nChannelIDIndex );
    }
    else
    {
        // Set Default message ID/Name, DLC and Databytes
        vSetDefaultValues();
    }
    UpdateData(TRUE);
    vUpdateStateDataBytes();
}

void CTxMsgDetailsView::vDisableSignalComponents()
{
    m_omLctrSigList.DeleteAllItems();
    // Disable Signal List
    m_omLctrSigList.EnableWindow(FALSE);
    if( m_odSignalMatrix.m_hWnd != nullptr )
    {
        // Reset the values
        m_odSignalMatrix.vResetValues();
        // Set the message Length
        m_odSignalMatrix.vSetMessageLength(0);
    }
}

void CTxMsgDetailsView::OnSelchangeCombChannelId()
{
    BOOL bEntryValid = FALSE;
    // Get the message IS
    int nMsgID = nGetValidMessageID();
    // IF it is valid
    if( nMsgID != -1)
    {
        // Get DLC value
        int nDLC =  (int)(m_odDLC.lGetValue());
        // If DLC Text is empty the above function call will return
        // 0. So make it -1 to denote invalid value
        if( nDLC == 0 )
        {
            CString omStrText;
            m_odDLC.GetWindowText(omStrText);
            if( omStrText.IsEmpty() == TRUE )
            {
                nDLC = -1;
            }
        }
        // If it is valid
        if( nDLC >= 0 && nDLC < 9 )
        {
            STCAN_MSG sCanInfo;
            // Update Message details from UI
            bUpdateMessageDetail(&sCanInfo);
            // Update Signal List information
            bUpdateSignalList(sCanInfo);
            // Update global message details
            bEntryValid = vUpdateSelectedMessageDetails();
        }
        vCallApplyChanges();
    }

    // Enable/Disable Add Button
    if( IsWindow(m_omButtonUpdateMsg.m_hWnd) )
    {
        vEnableAddButton( bEntryValid );
    }
    // Clear Error Text
    if( bEntryValid )
    {
        bSetStatusText("");
    }
}

void CTxMsgDetailsView::vSetDefaultValues()
{
    // Set Message ID/Name
    m_omComboMsgIDorName.SetWindowText("");

    // Set Databytes
    CString omStrDataBytesValue;

    // Two digits in case of Hex mode
    // Three digits in case of Dec mode
    BOOL bIsHex = CTxMsgManager::s_TxFlags.nGetFlagStatus(TX_HEX);
    if( bIsHex )
    {
        omStrDataBytesValue.Format(defFORMAT_DATA_HEX,0);
    }
    else
    {
        omStrDataBytesValue.Format(defFORMAT_DATA_DECIMAL,0);
    }
    // Set Data Byte Values
    m_odDB1.SetWindowText( omStrDataBytesValue );
    m_odDB2.SetWindowText( omStrDataBytesValue );
    m_odDB3.SetWindowText( omStrDataBytesValue );
    m_odDB4.SetWindowText( omStrDataBytesValue );
    m_odDB5.SetWindowText( omStrDataBytesValue );
    m_odDB6.SetWindowText( omStrDataBytesValue );
    m_odDB7.SetWindowText( omStrDataBytesValue );
    m_odDB8.SetWindowText( omStrDataBytesValue );

    // Set DLC
    m_odDLC.vSetValue(8);

    // Set the selection to Channel 1
    m_omComboChannelID.SetCurSel(0);
    vUpdateStateDataBytes();
}

BOOL CTxMsgDetailsView::PreTranslateMessage(MSG* pMsg)
{
    BOOL bSkip = FALSE;
    // If it is key press message
    if( pMsg->message == WM_KEYDOWN )
    {
        // If it is Enter Key
        if( pMsg->wParam == VK_RETURN )
        {
            // Send message to set focus to next control in tab order
            SendMessage(WM_NEXTDLGCTL, 0, 0);
            // Skip processing this message
            bSkip = TRUE;
        }
    }
    // If it is not Enter Key Press call parent function to handle
    if( bSkip == FALSE )
    {
        // Use same member to get the result
        bSkip = CFormView::PreTranslateMessage(pMsg);
    }
    // Send the result
    return bSkip;
}

void CTxMsgDetailsView::vCallApplyChanges()
{
    //update the global list for storing the changed data******************
    int             nCurrentIndex = -1;
    CTxMsgBlocksView* pomBlockView = nullptr;
    CTxMsgListView* pomListView = nullptr;
    pomBlockView = (CTxMsgBlocksView*)pomGetBlocksViewPointer();
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    PSMSGBLOCKLIST psMsgCurrentBlock = nullptr;
    // Get current block pointer
    psMsgCurrentBlock =
        pomBlockView->psGetMsgBlockPointer(
            pomBlockView->m_nSelectedMsgBlockIndex,
            pomBlockView->m_psMsgBlockList );

    if(psMsgCurrentBlock != nullptr)
    {
        PSTXCANMSGLIST pCanMsgList = psMsgCurrentBlock->m_psTxCANMsgList;

        pCanMsgList =  pomListView->psGetMsgDetailPointer(
                           pomListView->m_nSelectedMsgIndex, psMsgCurrentBlock);
        if(pCanMsgList != nullptr)
        {
            pCanMsgList->m_bModified = true;
        }
    }
    //***********************************************************************
    CTxFunctionsView* pView =
        ( CTxFunctionsView* )pomGetFunctionsViewPointer();
    if( pView != nullptr )
    {
        if(pView->m_CheckBoxAutoUpdate.GetCheck() == BST_CHECKED)
        {
            pView->vAccessButtonApply();

            CString strMsgIdName = "";
            GetDlgItemTextA(IDC_COMB_MSG_ID_NAME, strMsgIdName);
        }
    }
}

void CTxMsgDetailsView::vAutoUpdateModifyChanges()
{
    CTxMsgListView* pomListView = nullptr;
    pomListView = (CTxMsgListView* )pomGetListViewPointer();

    int             nCurrentIndex = -1;
    nCurrentIndex = pomListView ->m_omLctrMsgList.GetNextItem(nCurrentIndex,
                    LVNI_SELECTED );
    if(nCurrentIndex != -1)
    {
        vCallApplyChanges();
    }
}


void CTxMsgDetailsView::OnBnClickedCheckEnrollcheck()
{
	// TODO: 在此添加控件通知处理程序代码
	bUpdateMessageRollCntCheckSum();
}


void CTxMsgDetailsView::OnCbnSelchangeComboChecksumType()
{
	// TODO: 在此添加控件通知处理程序代码
	bUpdateMessageRollCntCheckSum();
}
