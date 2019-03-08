var baseUrl = "http://ktechpit.com/USS/Olivia/"

$(document).bind("mobileinit", function(){
        $.mobile.defaultPageTransition = 'slidefade';
        $.mobile.ajaxEnabled = true;
        $.mobile.hideUrlBar = false;
});

//persist navbar
$(function() {
    $( "[data-role='navbar']" ).navbar();
    $( "[data-role='header']" ).toolbar();
 });




// Update the contents of the toolbars
$( document ).on( "pageshow", "[data-role='page']", function() {
    // Each of the four pages in this demo has a data-title attribute
    // which value is equal to the text of the nav button
    // For example, on first page: <div data-role="page" data-title="Info">
    var current = $( this ).jqmData( "title" );
    // Change the heading
    $( "[data-role='header'] h1" ).text( current );
    // Remove active class from nav buttons
    $( "[data-role='navbar'] a.ui-btn-active" ).removeClass( "ui-btn-active" );
    // Add active class to current nav button
    $( "[data-role='navbar'] a" ).each(function() {
        if ( $( this ).text() === current ) {
            $( this ).addClass( "ui-btn-active" );
        }
    });
});


function showLoading() {
     $.mobile.loading("show", {
        text: "Loading..",
        textVisible: false,
        theme: "b",
        overlay: true
    });
}




var title,artist,album,coverUrl,songId,albumId,artistId,millis;
var base64; //returns base64 versio of album art to c++
var colorThief ; // colorThief object init
var dominantColor; //global
var html_data; //global html_data for youtube search
var album_loaded = false;
var track_loaded = false;
var artist_loaded = false;


$(document).ready(function($) {

    $(function () {
        $("[data-role=popup]").popup().enhanceWithin();
    });

    $(function() {
           $.mobile.defaultHomeScroll = 0;
    });

    $("#coverImage").load(function(){
          dominantColor = colorThief.getColor(document.querySelector("#coverImage"));
    });
});



function capitalize(str) {
    strVal = '';
    str = str.split(' ');
    for (var chr = 0; chr < str.length; chr++) {
      strVal += str[chr].substring(0, 1).toUpperCase() + str[chr].substring(1, str[chr].length) + ' '
    }
    return strVal;
}

//  core functions -------------

//albumId = albumList.at(0);
//albumName = albumList.at(1);
//base64 = albumList.at(2);
//dominantColor = albumList.at(3);
//artistName = albumList.at(4);
//artistId = albumList.at(5);
//tracksCount = albumList.at(6);

function open_saved_albums(){
    $("#saved_albums_result").empty();
    showLoading();
    $.mobile.changePage($('#albums_page'));
    var json = JSON.parse(store.web_print_saved_albums()); //albums Data is returned in json format
    var $html = "";
    $( ".ui-page-active [data-role='header'] h1" ).html(json.length+" saved albums");
    for(var i= 0; i < json.length;i++){
        $html = $html+
            "<li onclick='album_view(\""+json[i].albumId+"\")' data-filtertext='"+json[i].albumName+" "+json[i].artistName+" "+json[i].albumId+"' ><a>"+
            "<img style='max-width:100px;max-height:144px;width=100px;height=100px;' id='' src='data:image/png;base64,"+json[i].base64+"' \>"+
                    "<p>"+
                        ""+json[i].albumName+
                        "<br>"+
                        "Artist: "+json[i].artistName+
                        "<br>"+
                        "Tracks: "+json[i].tracksCount+
                    "</p>"+
               " </a>"+
            "</li>";
    }
    $.mobile.loading("hide");
    $("#saved_albums_result").append($html).listview("refresh");
    $('#albums_page .ui-content').trigger('create');
    $('#albums_page .ui-content').fadeIn('slow');
}

$(document).on("pagecreate", "#albums_page", function(){
    $('#a-search, #closeSearch').on('vclick', function (event) {
        $('#albumsfilter-input-form').toggleClass('moved');
    });
});

function album_view(id){
    $("#album_view_page .ui-content #saved_tracks_result").empty();
    showLoading();
    $.mobile.changePage($('#album_view_page'));
    var json = JSON.parse(store.web_print_album_tracks(id)); //albums Data is returned in json format
    var $html = "";
    $( ".ui-page-active [data-role='header'] h1" ).html(json.length+" downloaded songs");
    for(var i= 0; i < json.length;i++){
        var imgHtml,para;
        if(json[i].albumId.includes("undefined-")){
            para = "<p style='margin-left: 7.5em;'>";
            imgHtml = "<img id='"+json[i].songId+"' style='max-width:178px;max-height:144px;width=178px;height=100px;' id='' src='data:image/png;base64,"+json[i].base64+"' \>";
        }else{
            para = "<p>";
            imgHtml = "<img id='"+json[i].songId+"' style='max-width:100px;max-height:144px;width=100px;height=100px;' id='' src='data:image/png;base64,"+json[i].base64+"' \>";
        }
         $html = $html+
            "<li onclick='mainwindow.playLocalTrack(\""+json[i].songId+"\")' data-filtertext='"+json[i].title+" "+json[i].album+" "+json[i].artist+"' ><a>"+
                        imgHtml+para+
                        ""+json[i].title+
                        "<br>"+
                        "Album: "+json[i].album+
                        "<br>"+
                        "Artist: "+json[i].artist+
                    "</p>"+
               " </a>"+
            "</li>";
    }
    $.mobile.loading("hide");
    $("#album_view_page #saved_tracks_result").append($html).listview("refresh");
    $('#album_view_page .ui-content').trigger('create');
    $('#album_view_page .ui-content').fadeIn('slow');
}



function toDataUrl(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        var reader = new FileReader();
        reader.onloadend = function() {
            callback(reader.result);
        }
        reader.readAsDataURL(xhr.response);
    };
    xhr.open('GET', url);
    xhr.responseType = 'blob';
    xhr.send();
}



function gettrackinfo(searchterm){
    colorThief = new ColorThief();
    var arr = searchterm.split("!=-=!")
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];


    var query = title.replace("N/A","")+" - "+artist.replace("N/A","")+" - "+album.replace("N/A","");

    console.log(query);

    showLoading();

    toDataUrl(coverUrl, function(myBase64) {
        base64 = myBase64;
        document.querySelector("#coverImage").setAttribute("src",coverUrl);
        $.ajax({
            url: baseUrl+"youtube.php",
                   type:"GET",
                   data:{
                        "query": query,
                        "millis": millis
                   },
            success: function(html) {
                html_data =html;
                mainwindow.addToQueue(html_data,title,artist,album,base64,dominantColor,songId,albumId,artistId);
                $.mobile.loading("hide");
            }
        });

    });
}




