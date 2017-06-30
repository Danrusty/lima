import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4

GroupBox {
  
  property alias fileUrl: file_dialog.fileUrl
  property alias fileName: filename_lbl.text
  property alias text: action_button.text
  
  signal selected()
  signal triggered()
  
  onTriggered: {
    analyzeFileFromUrl(fileUrl)
  }
  
  id: select_file_menu
  
  title: "Analyser un fichier"
  
  FileDialog {
    id: file_dialog
    
    onAccepted: {
      filename_lbl.text = fileUrl
      selected()
    }
  }
  
  Row {
    spacing: 20
    
    Button {
      id: browse_button
      text: "Parcourir"
      
      onClicked: {
        file_dialog.open()
      }
    }
  
    Text {
      id: filename_lbl
      text: "..."
      width: 100
      elide: Text.ElideMiddle
      height: browse_button.height
    }
    
    Button {
      id: action_button 
      text: "Analyser"
      onClicked:  { 
        triggered()
      }
    }
    
  }
  
  
  
}
