var baseUrl = "http://ktechpit.com/USS/Olivia/"

$(document).bind("mobileinit", function(){
        $.mobile.defaultPageTransition = 'slidefade';
        $.mobile.ajaxEnabled = false;
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

$(document).ready(function($) {

    colorThief = new ColorThief();

    $(function () {
        $("[data-role=popup]").popup().enhanceWithin();
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
var categories_loaded = false;
var track_loaded = false;
var artist_loaded = false;
var overview_loaded = false;
var currentAlbumId = "null";

var pageType = "null";
var currentCategoryId = "null";

var currentPlaylistId = "null";



//onclicks functions
$(document).on("click","#overview",function(){
    if(!overview_loaded){
        overview();
    }
});


//  core functions -------------
function overview(){
    currentAlbumId = "undefined";
    showLoading();
    $("#pageloader i").text("Loading content please wait...");
    $("#overview_page .ui-loader-overview").fadeIn("slow");
    if(paginator.isOffline("browse","overview","null")){
        $("#overview_page .ui-loader-overview").fadeOut("slow");
        var html = paginator.load("browse","overview","null");
        $.mobile.loading("hide");
        overview_loaded = true;
        $('#overview_page .ui-content').html(html);
        $('#overview_page .ui-content').trigger('create');
        $('#overview_page .ui-content').fadeIn('slow');

        //set country by user's ip if is not set by user
        if(youtube.getCurrentCountry().length>0){
             //$('#currentCountry').text(youtube.getCurrentCountry());
        }else{
            getCountry();
        }
    }
    else{
        $.ajax({
            url: baseUrl+"overview.php",
            success: function(html) {
                $("#overview_page .ui-loader-overview").fadeOut("slow");
                paginator.save("browse","overview","null",html);
                $.mobile.loading("hide");
                overview_loaded = true;
                $('#overview_page .ui-content').html(html);
                $('#overview_page .ui-content').trigger('create');
                $('#overview_page .ui-content').fadeIn('slow');

                //set country by user's ip if is not set by user
                if(youtube.getCurrentCountry().length>0){
                     //$('#currentCountry').text(youtube.getCurrentCountry());
                }else{
                    getCountry();
                }
            },error: function(){
                overview_loaded = false;
                $("#pageloader i").text("An error occured, Unable to connect to host.");
            }
        });
    }
}


function track_option(track_id){
   // var channelHref = $('#'+track_id).parent().attr("data-channelhref");
    var searchterm = $('#'+track_id).parent().attr("onclick").split("gettrackinfo(\"")[1].split("\");")[0];
    var arr = searchterm.split("!=-=!")
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];


    //onclick=\''+$('#'+track_id).parent().attr("onclick")+'\'
    //https://www.youtube.com/watch?v=eqBkiu4M0Os
    var target = $( this ),
            options = '<hr><ul style="padding-bottom:5px" data-inset="true">'+
                        '<li>'+
                            '<a href="#" onclick="setPlaylistBaseId(\''+songId+'\')" >Show recommended tracks</a>'+
                        '</li>'+
                        '<li>'+
                            '<a href="#" id="'+songId+'_addToQueue" >Add to queue</a>'+
                        '</li>'+
                        '<li>'+
                            '<a href="#" onclick="album_view(\''+albumId+'\')" >Go to Album</a>'+
                        '</li>'+
                      '</ul>',
                link = "<span >id: "+ songId+"</span>",
                closebtn = '<a href="#" data-rel="back" class="ui-btn ui-corner-all ui-icon-delete ui-btn-icon-notext ui-btn-right">Close</a>',
                header = '<div style="margin: -12px -12px 0px -12px;" data-role="header"><h2>Options</h2></div>',
                img = '<img style="padding: 20px 0px 10px 0px;max-width: 200px;" src="'+coverUrl+'" alt="' + title + '" class="photo">',
                details = $('#'+track_id).parent().find("p")[0].outerHTML,
                popup = '<div data-history="false" style="text-align:center;padding:12px 12px; max-width:400px" data-transition="slideup" data-overlay-theme="b" data-dismissible="true" data-position-to="window" data-role="popup" id="popup-' + songId + '" data-short="' + songId +'"  data-corners="false" data-tolerance="15"></div>';
            $( link ).appendTo($( details ));
            // Create the popup.
            $( header )
                .appendTo( $( popup )
                .appendTo( $.mobile.activePage )
                .popup() )
                .toolbar()
                .before( closebtn )
                .after( img + details + options);
                $( "#popup-" + songId ).find('p').attr('style',"word-wrap: break-word;");
                $( "#popup-" + songId ).find("ul").listview();
                $( "#popup-" + songId ).popup( "open" ).trigger("create");
                $('body').css('overflow','hidden');

        $("#"+songId+"_addToQueue").on("click",function(){
                $( this ).parent().parent().parent().parent().parent().find("#"+songId).click();
                $( '#popup-'+songId ).remove();
                $('body').css('overflow','auto');
        });

        $( document ).on( "popupbeforeposition", $('#popup-'+songId ), function() {
            $( '#popup-'+songId).find("ul").listview();
            $('body').css('overflow','hidden');
        });

        // Remove the popup after it has been closed
        $( document ).on( "popupafterclose", $('#popup-'+songId), function() {
            $( '#popup-'+songId ).remove();
            $('body').css('overflow','auto');
        });
}

function gettrackinfo(searchterm){
    var videoId;
    var arr = searchterm.split("!=-=!")
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];

    if(albumId.includes("undefined")){ //yt case
        videoId = arr[4];
    }

    var query = title.replace("N/A","")+" - "+artist.replace("N/A",""); //+" - "+album.replace("N/A","")

  //  console.log(query);

    showLoading();

    toDataUrl(coverUrl, function(myBase64) {
        base64 = myBase64;
        document.querySelector("#coverImage").setAttribute("src",coverUrl);
        var img = document.querySelector("#coverImage");
        if (!img.complete) {
            img.addEventListener('load', function handler(e) {
                e.currentTarget.removeEventListener(e.type, handler);
                dominantColor = colorThief.getColor(img);
                if(albumId.includes("undefined")){
                    mainwindow.addToQueue(videoId+"<br>",title,artist,album,base64,dominantColor,songId,albumId,artistId);
                    $.mobile.loading("hide");
                }else{
                    $.mobile.loading("hide");
                   //we passing millis instead of ytids at first item in the function below
                    mainwindow.addToQueue(millis,title,artist,album,base64,dominantColor,songId,albumId,artistId);
                }
              });
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



//FUNCTIONS USED FOR NEW RELEASE=======================================
function getCountry(){
    $.get("https://ipinfo.io", function(response) {
        youtube.saveGeo(response.country);
    }, "jsonp");
}


function open_new_release(){
    currentAlbumId = undefined;
    if(!album_loaded){
        get_new_release("null"); //empty url for init
    }else{
        $.mobile.changePage($('#new_releases_page'));
    }
}

function open_categories(){
    currentAlbumId = undefined;
    if(!categories_loaded){
        get_categories("null"); //empty url for init
    }else{
        $.mobile.changePage($('#categories_page'))
    }
}

function get_categories(url){
    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }
    showLoading();
    $.mobile.activePage.find("#pageloader").fadeOut();
    if(paginator.isOffline("browse","get_categories",url)){
        $.mobile.changePage($('#categories_page'))
        $("#footer_categories").fadeIn("slow");
        $.mobile.loading("hide");
        var html = paginator.load("browse","get_categories",url);
        pageType = "categories_page";
        $("#categories_result").empty();
        $('html, body').stop().animate({ scrollTop : 0 }, 500);
        $("#categories_result").append(html).listview("refresh");
        $("#categories_result").fadeIn("slow");
        $('#categories_page .ui-content').trigger('create');
        categories_loaded = true;
    }else{
        $.ajax({
           url: baseUrl+"spotify/featured_categories.php",
                  type:"GET",
                  data:{
                       "country": country,
                       "url": url
                  },
           success: function(html) {
               $.mobile.changePage($('#categories_page'))
               $("#footer_categories").fadeIn("slow");
               $.mobile.loading("hide");
               paginator.save("browse","get_categories",url,html);
               pageType = "categories_page";
               $("#categories_result").empty();
               $('html, body').stop().animate({ scrollTop : 0 }, 500);
               $("#categories_result").append(html).listview("refresh");
               $("#categories_result").fadeIn("slow");
               $('#categories_page .ui-content').trigger('create');
               categories_loaded = true;
           },error: function(){
               categories_loaded = false;
               $.mobile.activePage.find("#pageloader i").text("An error occured, Unable to connect to host.");
               $.mobile.activePage.find("#pageloader").fadeIn();
           }
       });
    }
}

function show_category_playlist(categoy_id,url){

    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }
    showLoading();
    $.mobile.activePage.find("#pageloader").fadeOut();
    if(paginator.isOffline("browse","show_category_playlist",categoy_id+"<==>"+url)){
        $.mobile.changePage($('#category_playlist_page'));
        $("#footer_playlists").fadeIn("slow");
        var html_ = paginator.load("browse","show_category_playlist",categoy_id+"<==>"+url);
        pageType = "show_category_playlist";
        $.mobile.loading("hide");
        $("#playlist_result").empty();
        $('html, body').stop().animate({ scrollTop : 0 }, 500);
        $("#playlist_result").append(html_).listview("refresh");
        $("#playlist_result").fadeIn("slow");
        $('#playlist_result .ui-content').trigger('create');
    }else{
        $.ajax({
           url: baseUrl+"spotify/get_featured_categories_playlist.php",
                  type:"GET",
                  data:{
                       "cat_id": categoy_id,
                       "url": url,
                       "country": country
                  },
           success: function(html) {
               $.mobile.changePage($('#category_playlist_page'));
               $("#footer_playlists").fadeIn("slow");
               $.mobile.loading("hide");
               paginator.save("browse","show_category_playlist",categoy_id+"<==>"+url,html);
               pageType = "show_category_playlist";
               $("#playlist_result").empty();
               $('html, body').stop().animate({ scrollTop : 0 }, 500);
               $("#playlist_result").append(html).listview("refresh");
               $("#playlist_result").fadeIn("slow");
               $('#playlist_result .ui-content').trigger('create');
           },error: function(){
               $.mobile.loading("hide");
               $.mobile.activePage.find("#pageloader i").text("An error occured, Unable to connect to host.");
               $.mobile.activePage.find("#pageloader").fadeIn();
           }
       });
    }
    $('body').css('overflow','auto');
}

function show_playlist(p_id,url){
    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }
    $.mobile.changePage($('#playlist_view_page'));
    showLoading();
    $.mobile.activePage.find("#pageloader").fadeOut();
    if(paginator.isOffline("browse","show_playlist",p_id+"<==>"+url)){
        $("#footer_playlist_view").fadeIn("slow");
        var html_ = paginator.load("browse","show_playlist",p_id+"<==>"+url);
        $.mobile.loading("hide");
        pageType = "show_playlist";
        $("#playlist_tracks_result").empty();
        $('html, body').stop().animate({ scrollTop : 0 }, 500);
        $("#playlist_tracks_result").append(html_).listview("refresh");
        $("#playlist_tracks_result").fadeIn("slow");
        $('#playlist_tracks_result .ui-content').trigger('create');
    }else{
        $.ajax({
           url: baseUrl+"spotify/get_playlist_tracks.php",
                  type:"GET",
                  data:{
                       "p_id":p_id,
                       "url": url,
                       "country": country
                  },
                   success: function(html) {
                       paginator.save("browse","show_playlist",p_id+"<==>"+url,html);
                       $("#footer_playlist_view").fadeIn("slow");
                       $.mobile.loading("hide");
                       pageType = "show_playlist";
                       $("#playlist_tracks_result").empty();
                       $('html, body').stop().animate({ scrollTop : 0 }, 500);
                       $("#playlist_tracks_result").append(html).listview("refresh");
                       $("#playlist_tracks_result").fadeIn("slow");
                       $('#playlist_tracks_result .ui-content').trigger('create');
                   },error: function(){
                       $.mobile.loading("hide");
                       $.mobile.activePage.find("#pageloader i").text("An error occured, Unable to connect to host.");
                       $.mobile.activePage.find("#pageloader").fadeIn();
                   }
       });
    }
 $('body').css('overflow','auto');
}

function back_to_playlists_page(){
    $.mobile.changePage($('#category_playlist_page'));
}

function back_to_categories_page(){
    $.mobile.changePage($('#categories_page'));
}

function get_new_release(url){
    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }
    showLoading();
    $.mobile.activePage.find("#pageloader").fadeOut();
    if(paginator.isOffline("browse","get_new_release",url)){
        $.mobile.changePage($('#new_releases_page'));
        $("#footer_new_releases").fadeIn("slow");
        $.mobile.loading("hide");
        var html = paginator.load("browse","get_new_release",url);
        pageType = "new_releases_page";
        $("#albums_result").empty();
         $('html, body').stop().animate({ scrollTop : 0 }, 500);
        $("#albums_result").append(html).listview("refresh");
        $("#albums_result").fadeIn("slow");
        $('#albums_page .ui-content').trigger('create');
        album_loaded = true;
    }else{
        $.ajax({
           url: baseUrl+"spotify/new_release.php",
                  type:"GET",
                  data:{
                       "country": country,
                       "url": url
                  },
           success: function(html) {
               $.mobile.changePage($('#new_releases_page'));
               $("#footer_new_releases").fadeIn("slow");
               $.mobile.loading("hide");
               paginator.save("browse","get_new_release",url,html);
               pageType = "new_releases_page";
               $("#albums_result").fadeOut("slow");
               $("#albums_result").empty();
                $('html, body').stop().animate({ scrollTop : 0 }, 500);
               $("#albums_result").append(html).listview("refresh");
               $("#albums_result").fadeIn("slow");
               $('#albums_page .ui-content').trigger('create');
               album_loaded = true;
           },error: function(){
               album_loaded = false;
               $.mobile.activePage.find("#pageloader i").text("An error occured, Unable to connect to host.");
               $.mobile.activePage.find("#pageloader").fadeIn();
           }
       });
    }
}



//  album view
function album_view(id){
    showLoading();
    if(paginator.isOffline("album_view","album_view",id))
    {
        $.mobile.changePage($('#album_view_page'));
        $.mobile.loading("hide");
        var html = paginator.load("album_view","album_view",id);
        $("#album_view_page .ui-content").html(html);
        $('#album_view_page .ui-content').trigger('create');
        $('#album_view_page .ui-content').fadeIn('slow');
        if(typeof(NowPlayingTrackId) !== "undefined"){
            setNowPlaying(NowPlayingTrackId);
        }
    }else{
         $.ajax({
            url: baseUrl+"album_view.php",
                   type:"GET",
                   data:{
                        "query":id
                   },
            success: function(html) {
                paginator.save("album_view","album_view",id,html);
                $.mobile.changePage($('#album_view_page'));
                $.mobile.loading("hide");
                $("#album_view_page .ui-content").html(html);
                $('#album_view_page .ui-content').trigger('create');
                $('#album_view_page .ui-content').fadeIn('slow');
                if(typeof(NowPlayingTrackId) !== "undefined"){
                    setNowPlaying(NowPlayingTrackId);
                }
            },error:function(){
                $.mobile.activePage.find("#pageloader i").text("An error occured, Unable to connect to host.");
                $.mobile.activePage.find("#pageloader").fadeIn();
            }
        });
    }
    $('body').css('overflow','auto');
}


$(document).on("pageshow","#new_releases_page",function(){
    if(typeof(currentAlbumId) !== "undefined" && currentAlbumId !== "null" ){
        if(!isElementInViewport($('#'+currentAlbumId))){
            $(document).scrollTop($('#'+currentAlbumId).offset().top-window.innerHeight/2);
        }
    }
});

$(document).on("pageshow","#category_playlist_page",function(){
    if(typeof(currentCategoryId) !== "undefined" && currentCategoryId !== "null" ){
        if(!isElementInViewport($('#'+currentCategoryId))){
            $(document).scrollTop($('#'+currentCategoryId).offset().top-window.innerHeight/2);
        }
    }
});

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

function setNextButton(url){
    if(url.length > 5){
        var classes = $.mobile.activePage.find("#prevBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#prevBtn").attr("class",classes+" ui-disabled");

        var newClasses = $.mobile.activePage.find("#nextBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#nextBtn").attr('class',newClasses);
        if(pageType==="categories_page"){
            $.mobile.activePage.find("#nextBtn").attr("onclick","get_categories('"+url+"')");
        }else if(pageType==="new_releases_page"){
            $.mobile.activePage.find("#nextBtn").attr("onclick","get_new_release('"+url+"')");
        }else if(pageType === "show_category_playlist"){
            $.mobile.activePage.find("#nextBtn").attr("onclick","show_category_playlist('"+currentCategoryId+"','"+url+"')");
        }else if(pageType === "show_playlist"){
            $.mobile.activePage.find("#nextBtn").attr("onclick","show_playlist('"+currentPlaylistId+"','"+url+"')");
        }
    }else{
        var classe = $.mobile.activePage.find("#nextBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#nextBtn").attr("class",classe+" ui-disabled");
    }
}

function setPreviousButton(url){
    if(url.length > 5){
        var classes = $.mobile.activePage.find("#nextBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#nextBtn").attr("class",classes+" ui-disabled");

        var newClasses = $.mobile.activePage.find("#prevBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#prevBtn").attr('class',newClasses);
        if(pageType==="categories_page"){
            $.mobile.activePage.find("#prevBtn").attr("onclick","get_categories('"+url+"')");
        }else if(pageType ==="new_releases_page"){
            $.mobile.activePage.find("#prevBtn").attr("onclick","get_new_release('"+url+"')");
        }else if(pageType === "show_category_playlist"){
            $.mobile.activePage.find("#prevBtn").attr("onclick","show_category_playlist('"+currentCategoryId+"','"+url+"')");
        }else if(pageType === "show_playlist"){
            $.mobile.activePage.find("#prevBtn").attr("onclick","show_playlist('"+currentPlaylistId+"','"+url+"')");
        }
    }else{
            var classe = $.mobile.activePage.find("#prevBtn").attr('class').replace("ui-disabled","");
            $.mobile.activePage.find("#prevBtn").attr("class",classe+" ui-disabled");
         }
}

function back_to_release_page(){
    $.mobile.changePage($('#new_releases_page'));
}
//FUNCTIONS USED FOR NEW RELEASE=======================================

