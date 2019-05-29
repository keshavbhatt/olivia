var baseUrl = "http://ktechpit.com/USS/Olivia/";

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


//  core functions -------------


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
    var videoId;
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

    if(albumId.includes("undefined")){ //yt case
        videoId = arr[4];
    }

    var query = title.replace("N/A","")+" - "+artist.replace("N/A",""); //+" - "+album.replace("N/A","")

    console.log(query);

    showLoading();

    toDataUrl(coverUrl, function(myBase64) {
        base64 = myBase64;
        document.querySelector("#coverImage").setAttribute("src",coverUrl);
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
                    html_data =html;
                    mainwindow.addToQueue(html_data,title,artist,album,base64,dominantColor,songId,albumId,artistId);
                    $.mobile.loading("hide");
                }
            });
        }
    });
}

function show_playOriginal()
{
    document.getElementById('playOriginal').style.visibility="visible";
}

function hide_playOriginal()
{
    document.getElementById('playOriginal').style.visibility="hidden";
}


function msToTime(s) {

  // Pad to 2 or 3 digits, default is 2
  function pad(n, z) {
    z = z || 2;
    return ('00' + n).slice(-z);
  }
  var ms = s % 1000;
  s = (s - ms) / 1000;
  var secs = s % 60;
  s = (s - secs) / 60;
  var mins = s % 60;
  var hrs = (s - mins) / 60;

  return pad(hrs) + ':' + pad(mins) + ':' + pad(secs);
}

function quoteEscape(string) {
  return string.replace(/'/g, '\\\\\"');
}


function setPlaylistBaseId(trackId){

    $("#tracks_result_view").html("");
    $("#recommendation_page_suggestions").html("");
    $("#BASETRACK").html("");

    var country;
    if(youtube.getCurrentCountry().length>0){
         country = youtube.getCurrentCountry()
    }else{
         country = "US";
    }

    var $ul = $("#tracks_result_view");
    var html = "";

    $.ajax({
        url: "https://api.magicplaylist.co/mp/create/"+trackId,
        type: "GET",
        crossDomain: true,
        data: {
            "country": country
        }
    })
    .then( function ( response ) {
        $("#result_div").css("visibility","visible");
        var title,artist,album,coverUrl,songId,albumId,artistId,millis;

        $.each( response, function ( i, val ){
            if(i === "data"){
                var albumArt300 = val['original']['album']['images'][1]['url'];
                var albumArt600 = val['original']['album']['images'][0]['url'];

                var basedOn =  val['original']['name'];
                var basedOnArtist =  val['original']['artists'][0]['name'];
                var basedOnAlbum = val['original']['album']['name'];
                var basedAlbumId = val['original']['album']['id'];
                var basedArtistId = val['original']['artists'][0]['id'];
                var basedMillis = val['original']['duration_ms'];


                $.mobile.activePage.find("#BASETRACK").text(basedOn+", "+basedOnArtist);
                $.mobile.activePage.find("#ALBUM_ART").attr("src",albumArt300);
                $.mobile.activePage.find("#HEADER_DIV").get(0).style.cssText=document.querySelector("#HEADER_DIV").style.cssText+"background: url('"+albumArt600+"');background-size: cover;";
                $.mobile.activePage.find("#playOriginalButton").click(function (){
                    gettrackinfo(quoteEscape(basedOn)+"!=-=!"+quoteEscape(basedOnArtist)+"!=-=!"+quoteEscape(basedOnAlbum)+"!=-=!"+albumArt300+"!=-=!"+trackId+"!=-=!"+basedAlbumId+"!=-=!"+basedArtistId+"!=-=!"+basedMillis);
                });
                var playlist = val['playlist'];

                for(var j=0;j<playlist.length;j++){
                     coverUrl = playlist[j]['album']['images'][1]['url'];
                     title = playlist[j]['name'];
                     artist = playlist[j]['artists'][0]['name'];
                     artistId = playlist[j]['artists'][0]['id'];
                     songId = playlist[j]['id'];
                     album = playlist[j]['album']['name'];
                     albumId = playlist[j]['album']['id'];
                     millis =  playlist[j]['duration_ms'];
                     var rel_date = playlist[j]['album']['release_date'];
                     var duration = msToTime(millis);

                    html += "<li>"+
                        "<a onclick='gettrackinfo(&apos;"+quoteEscape(title)+"!=-=!"+quoteEscape(artist)+"!=-=!"+quoteEscape(album)+"!=-=!"+coverUrl+"!=-=!"+songId+"!=-=!"+albumId+"!=-=!"+artistId+"!=-=!"+millis+"&apos;)'>"+
                        "<img style='max-width:144px  !important ;max-height:144px' id='"+songId+"' src='"+coverUrl+"'>"+
                        "<p>"+
                           " Title: "+title+
                           " <br>"+
                            "Duration: "+duration+
                            "<br>"+
                            "Album: "+album +
                            "<br>"+
                            "Artist: "+artist+
                            "<br>"+
                            "Release Date: "+rel_date+
                        "</p>"+
                    "</a>"+
                   "</li>"
                }
            }
        });
        $ul.html( html );
        $ul.listview( "refresh" );
        $ul.trigger( "updatelayout");
    });
}



$(document).on("pagebeforeshow","#recommendation_page",function(){
    $('#manual_search').keydown(function(event){
        var keycode = (event.keyCode ? event.keyCode : event.which);
        if(keycode == '13'){
            forceSearch($("#manual_search").val());
            event.preventDefault();
            event.stopPropagation();
        }
    });
});


$( document ).on( "pagecreate", "#recommendation_page", function() {
    $( "#recommendation_page_suggestions" ).on( "filterablebeforefilter", function ( e, data ) {
        var $ul = $(this),
            $input = $( data.input ),
            value = $input.val(),
            html = "";
            $ul.html( "" );
        if ( value && value.length > 2 ) {
            $ul.html( "<li><div class='ui-loader'><span class='ui-icon ui-icon-loading'>loading..</span></div></li>" );
            $ul.listview( "refresh" );
            $("#result_div").css("visibility","hidden");
            $.ajax({
                url: "https://api.magicplaylist.co/mp/search",
                type: "GET",
                crossDomain: true,
                data: {
                    txt: $input.val()
                }
            })
            .then( function ( response ) {
                $.each( response, function ( i, val ) {
                    for(var j=0;j<val.length;j++){
                        var title = val[j]['name'];
                        var artist = val[j]['artists'][0]['name'];
                        var trackid = val[j]['id'];
                        html += '<li><a onclick="setPlaylistBaseId(\''+trackid+'\');" >'+title+', '+artist+'</a></li>';
                    }            });
                $ul.html( html );
                $ul.listview( "refresh" );
                $ul.trigger( "updatelayout");
            });
        }
    });
});

function forceSearch(val){
    var $ul =  $("#recommendation_page_suggestions"),
        value = val,
        html = "";
        $ul.html( "" );
    if ( value && value.length > 2 ) {
        $ul.html( "<li><div class='ui-loader'><span class='ui-icon ui-icon-loading'>loading..</span></div></li>" );
        $ul.listview( "refresh" );
        $.get("https://api.magicplaylist.co/mp/search?txt=haunted", function(response) {
            $.each( response, function ( i, val ) {
                console.log(i,val);
                for(var j=0;j<val.length;j++){
                    var title = val[j]['name'];
                    var artist = val[j]['artists'][0]['name'];
                    var trackid = val[j]['id'];
                    html += '<li><a onclick="setPlaylistBaseId(\''+trackid+'\');" >'+title+', '+artist+'</a></li>';
                }
                $ul.html( html );
                $ul.listview( "refresh" );
                $ul.trigger( "updatelayout");
            });
            }, "json");
    }
}

function getCountry(){
    $.get("https://ipinfo.io", function(response) {
        youtube.saveGeo(response.country);
    }, "jsonp");
}
