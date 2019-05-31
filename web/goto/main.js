var baseUrl = "http://ktechpit.com/USS/Olivia/"

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

function open_album_search(){
    $.mobile.changePage($('#albums_page'));
    var album_search_term = mainwindow.getTerm();
    if(!album_loaded){
        album_search(album_search_term);
    }
}

function open_track_search(){
    $.mobile.changePage($('#tracks_page'));
    var track_search_term = mainwindow.getTerm();
    if(!track_loaded){
        track_search(track_search_term);
    }
}

function open_artist_search(){
    $.mobile.changePage($('#artists_page'));
    var artist_search_term = mainwindow.getTerm();
    if(!artist_loaded){
        artist_search(artist_search_term);
    }
}




function track_search(term){
     showLoading();
     $.ajax({
        url: baseUrl+"search.php",
               type:"GET",
               data:{
                    "query":term
               },
        success: function(html) {
            $.mobile.loading("hide");
            $("#tracks_result").append(html).listview("refresh");
            $('#tracks_page .ui-content').trigger('create');
            $('#tracks_page .ui-content').fadeIn('slow');
            track_loaded = true;
            mainwindow.resultLoaded();
        }
    });
}


function album_search(term){
     showLoading();
     $.ajax({
        url: baseUrl+"album_search.php",
               type:"GET",
               data:{
                    "query":term
               },
        success: function(html) {
            $.mobile.loading("hide");
            $("#albums_result").append(html).listview("refresh");
            $('#albums_page .ui-content').trigger('create');
            $('#albums_page .ui-content').fadeIn('slow');
            album_loaded = true;
            mainwindow.resultLoaded();
        }
    });
}

function artist_search(term){
     showLoading();
     $.ajax({
        url: baseUrl+"artist_search.php",
               type:"GET",
               data:{
                    "query":term
               },
        success: function(html) {
            $.mobile.loading("hide");
            $("#artists_result").append(html).listview("refresh");
            $('#artists_page .ui-content').trigger('create');
            $('#artist_page .ui-content').fadeIn('slow');
            artist_loaded = true;
            mainwindow.resultLoaded();
        }
    });
}


function album_view(id){
     showLoading();
    if(paginator.isOffline("album_view","album_view",id))
    {
        var html = paginator.load("album_view","album_view",id);

        $.mobile.loading("hide");
        $.mobile.changePage($('#album_view_page'));
        $("#album_view_page .ui-content").html(html);
        $('#album_view_page .ui-content').trigger('create');
        $('#album_view_page .ui-content').fadeIn('slow');

        setNowPlaying(NowPlayingTrackId);
        $("#"+NowPlayingTrackId).css("cssText","position: absolute;left: 0px;top: 0px;");
        $(".nowPlaying").css("cssText","width:"+$("#"+NowPlayingTrackId).width()+"px");

    }else{
         $.ajax({
            url: baseUrl+"album_view.php",
                   type:"GET",
                   data:{
                        "query":id
                   },
            success: function(html) {
                paginator.save("album_view","album_view",id,html);

                $.mobile.loading("hide");
                $.mobile.changePage($('#album_view_page'));
                $("#album_view_page .ui-content").html(html);
                $('#album_view_page .ui-content').trigger('create');
                $('#album_view_page .ui-content').fadeIn('slow');
                setNowPlaying(NowPlayingTrackId);
            }
        });
    }
}

function artist_view(id){
     showLoading();
     $.ajax({
        url: baseUrl+"itunes_artist_bio.php",
               type:"GET",
               data:{
                    "artistId":id
               },
        success: function(html) {
            $.mobile.loading("hide");
            $.mobile.changePage($('#artist_view_page'));
            $("#artist_view_page .ui-content").html(html);
            $('#artistview_page .ui-content').trigger('create');
            $('#artist_view_page .ui-content').fadeIn('slow');
        }
    });
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

    var query = title.replace("N/A","")+" - "+artist.replace("N/A",""); //+" - "+album.replace("N/A","")

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

//called from album_view.php
function setAlbumMeta(album,album_art,album_art_header,artist,tracks_count,copyright
                     ,genere,release_date){
    $.mobile.activePage.find("#ALBUM").text(album);
    $.mobile.activePage.find("#ALBUM_ART").attr("src",album_art);
    $.mobile.activePage.find("#HEADER_DIV").get(0).style.cssText=document.querySelector("#HEADER_DIV").style.cssText+"background: url('"+album_art_header+"');background-size: cover;";
    $.mobile.activePage.find("#ARTIST").text(artist);
    $.mobile.activePage.find("#TRACKS_COUNT").text(tracks_count);
    $.mobile.activePage.find("#COPYRIGHT").text(copyright);
    $.mobile.activePage.find("#PRIMARY_GENRE").text(genere);
    $.mobile.activePage.find("#RELEASE_DATE").text(release_date);
}





