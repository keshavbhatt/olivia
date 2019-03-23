$(document).bind("mobileinit", function(){
      $.mobile.defaultPageTransition = 'slidefade';
      $.mobile.defaultDialogTransition = 'pop';
      $.mobile.useFastClick = true;
});


//include this lib after loading jquery

if (!String.prototype.includes) {
    String.prototype.includes = function() {
        'use strict';
        return String.prototype.indexOf.apply(this, arguments) !== -1;
    };
}


var themeColor; //sets the global themeColor for webview is accsible from all web
var NowPlayingTrackId;

function func() {

}

function hideLoading(){
    $.mobile.loading("hide");
    mainwindow.showAjaxError();
}

$( document ).ajaxError(function() {
    hideLoading();
});

function changeBg(rgba){ // sets bg color var in mainwindow and changes bg color of webview
    themeColor = rgba;
    var arr = rgba.split(",");
    var r = arr[0];
    var g = arr[1];
    var b = arr[2];
    var a = arr[3];
    $("body").css("cssText","background-color:rgba("+rgba+") !important;");
//    $(".ui-header-fixed .ui-btn").css("cssText","background-color:rgba("+rgba+") !important;");
//    $("html .ui-bar-b .ui-btn.ui-btn-active").css("cssText","background-color:rgba("+r+','+g+','+b+','+'0.5'+") !important;"); // dark
    $(".ui-listview>.ui-li-divider").css("cssText","background-color:rgba("+rgba+") !important;");
//    $(".ui-input-search.ui-body-inherit.ui-corner-all.ui-shadow-inset.ui-input-has-clear").css("cssText","background-color:rgba("+r+','+g+','+b+','+'0.4'+") !important;"); //dark
    mainwindow.setThemeColor(themeColor);
}

function setNowPlaying(songId){ //nowPlaying styles are in main.css
    //removes all now playing

    $(".nowPlaying").remove();
    $.mobile.activePage.remove(".nowPlaying");

    var songIdStr = "#"+songId;

    var maxWidth;
    if($(songIdStr).css("max-width")!== "undefined"){
        maxWidth = $(songIdStr).css("max-width");
    }else{
        maxWidth = "100px";
    }

    //adds nowPlaying pages
    $(songIdStr).css("cssText","position: absolute;left: 0px;top: 0px;max-width:"+maxWidth+"; width:"+maxWidth+"; ");
    $(songIdStr).each(function( index ) {
      $( this ).before("<div style='max-width:"+maxWidth+"; width:"+maxWidth+";' class='nowPlaying'></div>");
    });

    //for album [adds nowPlaying active page]
    $.mobile.activePage.find(songIdStr).css("cssText","position: absolute;left: 0px;top: 0px;max-width:"+maxWidth+"; width:"+maxWidth+";");
    $.mobile.activePage.find(songIdStr).before("<div style='max-width:"+maxWidth+"; width:"+maxWidth+";' class='nowPlaying'></div>");

}


//function escape_str( str ) {
//    return (str + '').replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
//}

