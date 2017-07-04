import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "scripts/DynamicObjectManager.js" as Dom
import "scripts/colors.js" as Colors

/// This is an attempt at an interactive tab bar with a specific design
/// that QML won't allow. Trying to find some workarounds...
/// Unused for now
/// It is an attempt to make this work with Qt Quick Controls 2

Rectangle {
  id: main_tab_bar
  
  color: fgcolor;
  ////
  
  property alias currentIndex: bar.currentIndex
  property color fgcolor: "#aaaaaa"
  property color bgcolor: "#777777"
  
  /// if true, will impose foreground and background colors to buttons
  property bool global_colors: true
  
  /// it is likely that we may need to hold the name and the contentItem in a specific class, rather than just the button
  property var buttons: []
  
  /// content Items
  property var items: []
  
  function addTab(name, contentItemSrc) {
    /// Button
    Dom.createComponent("CustomTabButton.qml", bar);
    var btn = Dom.obj;
    bar.addItem(btn);
    btn.x = 0;
    btn.y = 0;
    
    btn.text = name;
    buttons.push(btn);
    btn.index = buttons.length - 1;
    
    /// Content 
    Dom.createComponent(contentItemSrc, container);
    
    items.push(Dom.obj);
    
    if (buttons.length != bar.count) {
      console.log("now that's a problem");
    }
    
    btn.selectedColor = Dom.obj.color
    btn.idleColor = Colors.darker(Dom.obj.color)
    
    
    
    if (buttons.length == 1) {
      select(0);
    }
    
    Dom.createComponent("basics/BasicRectangle.qml", bar)
    Dom.obj.parent = bar
    Dom.obj.width = 3
    Dom.obj.height = bar.height
    
  }
  
  
  function select(index) {
    if (index >= 0 && index < buttons.length) {
      for (var i = 0; i < buttons.length; i++) {
        if (global_colors) {
          buttons[i].color = bgcolor;
        }
        else {
          buttons[i].color = buttons[i].idleColor;
        }
      }
      if (global_colors) {
        buttons[index].color = fgcolor;
      }
      else {
        buttons[index].color = buttons[index].selectedColor;
      }
      currentIndex = index;
    }
  }
  
  function closeTab(index) {
    // call confirmation popup
    if (index >= 0 && index < buttons.length) {
      items[index].destroy()
      bar.removeItem(buttons[index])
      buttons[index].destroy()
      buttons.splice(index, 1)
      
      currentIndex = buttons.length > 0 ? buttons.length - 1 : 0
    }
    
  } 
  
  TabBar {
    id: bar
    objectName: "tabbar's tabbard"
    property alias color: tabbedview_tabbar_bg.color
    color: "#333333"
    
    width: parent.width
    
    background: Rectangle {
      id: tabbedview_tabbar_bg
      anchors.fill: parent
    }
    
    Rectangle {
      color:"red"
      width: 4
      height: parent.height
      x: 0
      y: 0
    }
    
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    //////////////////////////////////////////////
    
  }
  
  StackLayout {
    id: container
    width: parent.width
    height: parent.height - bar.height 
    y: bar.height + 5
    currentIndex: bar.currentIndex
    
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    /////////////////////////////////////////////
    
//     Rectangle {
//       color:"#ee00eeaa"
//     }
  }
}
