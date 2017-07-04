import QtQuick 2.0
import QtQuick.Controls 1.4
import "basics"

/// Widget allowing to input text and analyze it on the fly

GroupBox {
  id: analyzeTextWidget
  
  anchors.fill: parent
  
  title: "Analyser du texte"
  
  Rectangle {
    id: text_bunch
    property alias text: textv.text
    
    width: parent.width
    height: parent.height - 50
    
    anchors.margins: 5

    TextEditor {
      id:textv
    }
  }
  
  Button {
    text:"Analyze Text"
    onClicked: {
      analyzeText(text_bunch.text)
    }
    enabled: textAnalyzer.ready ? true : false
    anchors.top: text_bunch.bottom
  }
  
  
  
}
