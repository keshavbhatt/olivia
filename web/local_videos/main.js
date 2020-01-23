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


$(document).on('keydown', function ( e ) {
    if ((e.metaKey || e.ctrlKey) && ( String.fromCharCode(e.which).toLowerCase() === 'f') ) {
        $('#songsfilter-input-form').toggleClass('moved');
        $("#songsfilter-input").focus();
    }
});



var title,artist,album,coverUrl,songId,albumId,artistId,millis;
var base64; //returns base64 versio of album art to c++
var colorThief ; // colorThief object init
var dominantColor; //global
var html_data; //global html_data for youtube search
var album_loaded = false;
var track_loaded = false;
var artist_loaded = false;


$(document).ready(function($) {
    colorThief = new ColorThief(); // colorThief object init

    $(function () {
        $("[data-role=popup]").popup().enhanceWithin();
    });

    $(function() {
           $.mobile.defaultHomeScroll = 0;
    });

//    $("#coverImage").load(function(){
//          dominantColor = colorThief.getColor(document.querySelector("#coverImage"));
//    });
});


//  core functions -------------
function open_local_saved_videos(){
    $("#saved_tracks_result").empty();
    showLoading();
    $.mobile.changePage($('#tracks_page'));
    $('.ui-content').hide();
    var json = JSON.parse(store.web_print_local_saved_videos()); //songs Data is returned in json format
    var $html = "";
    $( ".ui-page-active [data-role='header'] h1" ).html(json.length+" downloaded videos");
    for(var i= 0; i < json.length;i++){
        var divider = "!=-=!";
//        var albumType = (json[i].album === "undefined") ? "Youtube":"";
        var imgHtml,para;
               if(json[i].albumId.includes("undefined-")){
                   para = "<p style='margin-left: 7.5em;'>";
                   imgHtml = "<img id='"+json[i].songId+"' style='max-width:178px;max-height:144px;width=178px;height=100px;' id='' src='data:image/png;base64,"+json[i].base64+"' \>";
               }else{
                   para = "<p style='margin-left: 14.5em;' >";
                   imgHtml = "<p style='background-color:rgb("+json[i].dominant+");' class='li-img-wrapper'><img id='"+json[i].songId+"' style='width:100%;max-width:100px;max-height:144px;width=100px;height=100px;' id='' src='data:image/png;base64,"+json[i].base64+"' \></p>";
               }
         $html = $html+
            "<li  data-filtertext='"+json[i].title+" "+json[i].album+" "+json[i].artist+"' >"+
            "<a  onclick='mainwindow.playVideo(\""+json[i].songId+"\")' data-trackinfo='"+json[i].title+divider+json[i].artist+divider+json[i].album+divider+json[i].base64+divider+json[i].songId+divider+json[i].albumId+divider+json[i].artistId+divider+"millis"+"'>"
         +imgHtml+para+
                        ""+json[i].title+
                        "<br>"+
                        "Album: "+json[i].album+
                        "<br>"+
                        "Artist: "+json[i].artist+
                    "</p>"+
//                  "<p class='ui-li-aside'>"+albumType+"</p>" +
               " </a>"+
             "<a href='#' onclick=\"track_option('"+json[i].songId+"')\">More Options</a>"+
            "</li>";
    }
    $("#saved_tracks_result").append($html).listview("refresh");
    $('#tracks_page .ui-content').trigger('create');
    $('#tracks_page .ui-content').fadeIn('slow');
    $.mobile.loading("hide");
}

$(document).on("pagecreate", "#tracks_page", function(){
    $('#a-search, #closeSearch').on('vclick', function (event) {
        $('#songsfilter-input-form').toggleClass('moved');
        $("#songsfilter-input").focus();
    });
});

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
        var img = document.querySelector("#coverImage");
        if (!img.complete) {
            img.addEventListener('load', function handler(e) {
                e.currentTarget.removeEventListener(e.type, handler);
                dominantColor = colorThief.getColor(img);
                $.mobile.loading("hide");
               //we passing millis instead of ytids at first item in the function below
                mainwindow.addToQueue(millis,title,artist,album,base64,dominantColor,songId,albumId,artistId);

            });
        }

    });
}

function track_option(track_id){
   // var channelHref = $('#'+track_id).parent().attr("data-channelhref");
    var searchterm ;
    if(typeof($("#"+track_id).parent().attr("data-trackinfo"))==="undefined"){
        searchterm = $("#"+track_id).parent().parent().attr("data-trackinfo");
    }else{
        searchterm = $("#"+track_id).parent().attr("data-trackinfo")
    }
    var arr = searchterm.split("!=-=!");
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    base64 = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];

    //onclick=\''+$('#'+track_id).parent().attr("onclick")+'\'
    //https://www.youtube.com/watch?v=eqBkiu4M0Os
    var target = $( this ),
            options = '<hr><ul style="padding-bottom:5px" data-inset="true">'+
                        '<li>'+
                            '<a href="#" onclick="delete_video_cache(\''+track_id+'\')" >Delete Video cache</a>'+
                        '</li>'+
                      '</ul>',
                link = "<span >id: "+ songId+"</span>",
                closebtn = '<a href="#" data-rel="back" class="ui-btn ui-corner-all ui-icon-delete ui-btn-icon-notext ui-btn-right">Close</a>',
                header = '<div style="margin: -12px -12px 0px -12px;" data-role="header"><h2>Options</h2></div>',
                img = '<img style="padding: 20px 0px 10px 0px;max-width: 60%;" src="data:image/png;base64,'+base64+'" alt="' + title + '" class="photo">',
                popup = '<div data-history="false" style="text-align:center;padding:12px 12px; max-width:400px" data-transition="slideup" data-overlay-theme="b" data-dismissible="true" data-position-to="window" data-role="popup" id="popup-' + songId + '" data-short="' + songId +'"  data-corners="false" data-tolerance="15"></div>';
                var details;
                if(typeof($('#'+track_id).parent().find("p")[0])==="undefined"){
                    details = $('#'+track_id).parent().parent().parent().find("p")[1].outerHTML;
                }else{
                    details = $('#'+track_id).parent().find("p")[0].outerHTML
                }
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

function delete_video_cache(track_id){
    $( '#popup-'+songId ).remove();
    $('body').css('overflow','auto');

    $("#"+songId).closest("li").remove();
    //keep the order below
   // mainwindow.remove_song(track_id);
    mainwindow.delete_video_cache(track_id);
}



