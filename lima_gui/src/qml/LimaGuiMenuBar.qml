import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

/// This is the menu bar of the main application window
/// Always stays on

MenuBar {
  
  style: MenuBarStyle {
    background: Rectangle {
      color:"white"
    }
    menuStyle: MenuStyle {
      frame : Rectangle {
        color:"white"
      }
      
    }
  }
  
  Menu {
    title: "Fichier"
    
    MenuItem {
      text: "Nouveau "
      shortcut: "Ctrl+N"
      onTriggered: {
        createNewElement()
      }
    }
    
    MenuItem { 
      text: "Ouvrir un fichier";
      shortcut: "Ctrl+O"
      onTriggered: {
        openSelectFileDialog()
      }
    }
    MenuItem { text: "Sauvegarder"; }
    MenuItem { text: "Sauvegarder en tant que ..."; }
    
    MenuItem {
      text: "Exporter ..."
      onTriggered: confirmCloseFile()
    }
    
    MenuItem {
      text: "Quitter"
      onTriggered:  Qt.quit()
      
    }
  }
  
  Menu {
    title: "Edition"
    
    MenuItem { text:"Couper" }
    MenuItem { text:"Copier" }
    MenuItem { text:"Coller" }
  }
  
  Menu {
    title: "Analyse"

    MenuItem {
      text: "Analyser fichier courant"

      enabled: textAnalyzer.ready && data_tab_view.count ? true : false
      onTriggered: {
        analyzeFile(data_tab_view.currentTab().title);
      }
    }
    
    MenuItem { text: "Analyser du texte"
      onTriggered: openAnalyzeTextTab();
    }
    
    MenuItem { text:"Analyser des fichiers"
      onTriggered: openAnalyzeFileTab();
    }
    MenuItem { text: "Analyse CONLL" }
    MenuItem { text: "Graphe" }
    MenuItem { text: "Entités nommées" }

    MenuItem { text:"Debug Test"
      onTriggered: {
        textAnalyzer.test()
      }
      shortcut:"Ctrl+T"
    }
  }
  
  Menu {
    title: "Lima"
    
    Menu {
      title: "Configurations"
      
      // that's where i'd put my preset configs .. if i had one !
      
      MenuItem {
        text: "default"
      }
    }
    
    MenuItem {
      text: "Configurer LIMA"
    }
    
    MenuItem {
      text:"Options"
    }
  }
}
