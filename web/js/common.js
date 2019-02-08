//include this lib after loading jquery

var themeColor; //sets the global themeColor for webview is accsible from all web

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


