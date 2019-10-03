var baseUrl = "http://ktechpit.com/USS/Olivia/"

$(document).bind("mobileinit", function(){
        $.mobile.defaultPageTransition = 'slide';
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
var track_loaded = false;
var artist_loaded = false;
var overview_loaded = false;
var currentAlbumId = "null";





//onclicks functions
$(document).on("click","#overview",function(){
    if(!overview_loaded){
        overview();
    }
});


//  core functions -------------
function overview(){
    $("#overview_page .ui-loader-overview").show();
    $('#overview_page .ui-content').fadeOut('slow');
    $.ajax({
        url: baseUrl+"overview.php",
        success: function(html) {
            overview_loaded = true;
            $('#overview_page .ui-content').fadeOut('slow');
            $('#overview_page .ui-content').html(html);
            $('#overview_page .ui-content').trigger('create').fadeIn('slow');
            $("#overview_page .ui-loader-overview").hide();
        },error: function(){
            overview_loaded = false;
        }
    });
}


function track_option(track_id){
   // var channelHref = $('#'+track_id).parent().attr("data-channelhref");
    var searchterm = $('#'+track_id).parent().attr("onclick").split("gettrackinfo(\'")[1].split(");")[0];
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

    console.log(query);

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
                    $.ajax({
                        url: baseUrl+"youtube.php",
                           type:"GET",
                           data:{
                            "query": query,
                            "millis": millis
                           },
                        success: function(html) {
                            html_data = html;
                            mainwindow.addToQueue(html_data,title,artist,album,base64,dominantColor,songId,albumId,artistId);
                            $.mobile.loading("hide");
                        }
                    });
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
    $.mobile.changePage($('#new_releases_page'));
    if(!album_loaded){
        get_new_release("null"); //empty url for init
    }
}

function get_new_release(url){
    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }
    showLoading();
    $.ajax({
       url: baseUrl+"spotify/new_release.php",
              type:"GET",
              data:{
                   "country": country,
                   "url": url
              },
       success: function(html) {
           $("#albums_result").empty();
           $("#footer_new_releases").show();
           $.mobile.loading("hide");
           $("#albums_result").append(html).listview("refresh");
           $('#albums_page .ui-content').trigger('create');
           $('#albums_page .ui-content').fadeIn('slow');
           album_loaded = true;
       },error: function(){
           album_loaded = false;
       }
   });
}

//  album view
function album_view(id){
    $.mobile.changePage($('#album_view_page'));
    $('#album_view_page .ui-content').html("");
    $('body').css('overflow','auto');
    showLoading();
    if(paginator.isOffline("album_view","album_view",id))
    {
        var html = paginator.load("album_view","album_view",id);

        $.mobile.loading("hide");
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
                $.mobile.loading("hide");
                $("#album_view_page .ui-content").html(html);
                $('#album_view_page .ui-content').trigger('create');
                $('#album_view_page .ui-content').fadeIn('slow');
                if(typeof(NowPlayingTrackId) !== "undefined"){
                    setNowPlaying(NowPlayingTrackId);
                }            }
        });
    }
}

$(document).on("pageshow","#new_releases_page",function(){
    if(typeof(currentAlbumId) !== "undefined" && currentAlbumId !== "null" )
        $(document).scrollTop($('#'+currentAlbumId).offset().top-40);
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
        $.mobile.activePage.find("#nextBtn").attr("onclick","get_new_release('"+url+"')");
    }
}

function setPreviousButton(url){
    if(url.length > 5){
        var classes = $.mobile.activePage.find("#nextBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#nextBtn").attr("class",classes+" ui-disabled");

        var newClasses = $.mobile.activePage.find("#prevBtn").attr('class').replace("ui-disabled","");
        $.mobile.activePage.find("#prevBtn").attr('class',newClasses);
        $.mobile.activePage.find("#prevBtn").attr("onclick","get_new_release('"+url+"')");
    }
}

function back_to_release_page(){
    $.mobile.changePage($('#new_releases_page'));
}
//FUNCTIONS USED FOR NEW RELEASE=======================================

