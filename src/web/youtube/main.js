var baseUrl = "http://ktechpit.com/USS/Olivia/"

$(document).bind("mobileinit", function(){
        $.mobile.defaultPageTransition = 'fade';
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
var colorThief;
var dominantColor; //global
var html_data; //global html_data for youtube search
var album_loaded = false;
var track_loaded = false;
var artist_loaded = false;


$(document).ready(function($) {

    colorThief = new ColorThief(); // colorThief object init

    $(function () {
        $('[data-role=popup]').popup().enhanceWithin();
    });

    $(function() {
           $.mobile.defaultHomeScroll = 0;
    });

    $("#manual_search").focus();
});


//  core functions -------------

function manual_youtube_search(term){
    $.mobile.changePage($('#manul_youtube_page'));
    $('.ui-content').hide();
    if(term===""){
        term = $("#manual_search").val();
    }
    showLoading();
    $("#result_div").html("");
    if(paginator.isOffline("youtube","manual_youtube_search",term))
    {
        var html = paginator.load("youtube","manual_youtube_search",term);
        $.mobile.loading("hide");
        $("#result_div").html(html);
        $('#manul_youtube_page .ui-content').trigger("create");
        $('#manul_youtube_page .ui-content').fadeIn('slow');
        $('#manul_youtube_page_suggestions').html("");
        mainwindow.checkForPlaylist(); //special case cause we are loading results on same page

    }else{
        $.ajax({
           url: baseUrl+"manual_youtube_search.php",
                  type:"GET",
                  data:{
                       "query":term
                  },
           success: function(html) {
               paginator.save("youtube","manual_youtube_search",term,html);

               $.mobile.loading("hide");
               $("#result_div").html(html);
               $('#manul_youtube_page .ui-content').trigger("create");
               $('#manul_youtube_page .ui-content').fadeIn('slow');
               $('#manul_youtube_page_suggestions').html("");
               mainwindow.checkForPlaylist(); //special case cause we are loading results on same page
           }
       });
    }
    $("#trending_div").hide();
    $("#history_div").hide();
}

//send base64 url of covers
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

   // console.log(query);

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

function setManualSearchVal(text){
    $("#manual_search").val(text);
    $("#manual_search").focus();
    $("#manul_youtube_page_suggestions" ).empty();
}

var blockNextFilter = false;
$(document).on("pagebeforeshow","#manul_youtube_page",function(){
    $("#manual_search").on('keydown', function ( e ) {
        var key = e.charCode ? e.charCode : e.keyCode ? e.keyCode : 0;
        if(key === 13) {
            blockNextFilter = true;
            e.preventDefault();
            manual_youtube_search($(this).val());
        }else{
            blockNextFilter = false;
        }
    });
});


$( document ).on( "pagecreate", "#manul_youtube_page", function() {
    $( "#manul_youtube_page_suggestions" ).on( "filterablebeforefilter", function ( e, data ) {
        if (blockNextFilter) {
            e.preventDefault();
            return;
          }

        var $ul = $(this),
            $input = $( data.input ),
            value = $input.val(),
            html = "";
        $ul.html( "" );
        if ( value && value.length > 2 ) {
            $ul.html( "<li><div class='ui-loader'><span class='ui-icon ui-icon-loading'>loading..</span></div></li>" );
            $ul.listview( "refresh" );
            $.ajax({
                url: "http://suggestqueries.google.com/complete/search?ds=yt&client=youtube&hjson=t&cp=1&format=5&alt=json",
                dataType: "jsonp",
                crossDomain: true,
                data: {
                    q: $input.val()
                }
            })
            .then( function ( response ) {
                $.each( response, function ( i, val ) {
                    for(var j=0;j<val.length;j++){
                        var term = val[j].toString().split(",")[0];
                        html += '<li style="cursor:pointer" onclick="setManualSearchVal(\''+term+'\');" >'+term+'</li>';
                    }
                });
                $ul.html( html );
                $ul.listview( "refresh" );
                $ul.trigger( "updatelayout");
            });
        }
    });
});


$(document).on('click', '#navBtn', function() {
    showLoading();
    var linkStr = $(this).attr("data-link");
    $('#result_div').fadeOut("slow");
    $.ajax({
        type: "GET",
        url: baseUrl+"manual_youtube_search.php",
        data: {
            nav : linkStr
        },
        success: function(html) {
            $.mobile.loading("hide");
            $("#result_div").html(html);
            $("#result_div").fadeIn("slow");
            $('#manul_youtube_page .ui-content').trigger("create");
            $('#manul_youtube_page_suggestions').fadeOut("slow");
            $('#manul_youtube_page_suggestions').html("");
            mainwindow.checkForPlaylist(); //special case cause we are loading results on same page
         }
        });
});

function orderChanged(linkStr,pageType){
    showLoading();
    $('#result_div').fadeOut("slow");
    $.ajax({
        type: "GET",
         url: baseUrl+"manual_youtube_search.php",
        data: {
            nav : linkStr
        },
        success: function(html) {
            if(pageType==="category"){
               orderHider = setInterval(function(){
                    $("#select-custom-14-button").fadeOut('slow');
                    $("#select-custom-15-button").fadeOut('slow');
                },300);
            }
            $.mobile.loading("hide");
            $("#result_div").html(html);
            $("#result_div").fadeIn("slow");
            $('#manul_youtube_page .ui-content').trigger("create");
            $('#manul_youtube_page_suggestions').fadeOut("slow");
            $('#manul_youtube_page_suggestions').html("");
            mainwindow.checkForPlaylist(); //special case cause we are loading results on same page
         }
        });
}

function load_history(){
    var html = paginator.getList("youtube","manual_youtube_search");
    if(html.length===0){
        $("#history_div").hide();
    }else{
        $("#history").html(html);
        $("#history_div").fadeIn("slow");
    }
    if(youtube.getCurrentCountry().length>0){
         $('#currentCountry').text(youtube.getCurrentCountry());
    }else{
        getCountry();
    }
    //get trending
    youtube_trending(youtube.getCurrentCountry());
}

function youtube_trending(country){
    if(country===""){
        country= "us";
    }
    $("#trending_div .ui-loader-trending").show();
    $("#trending").hide();
    if(paginator.isOffline("youtube","youtube_trending",country))
    {
        var html_ = paginator.load("youtube","youtube_trending",country);
        $("#trending_div .ui-loader-trending").hide();
        $.mobile.loading("hide");
        $("#trending").html(html_);
        $("#trending").fadeIn("slow");
        $.mobile.activePage.find("#trending").trigger("create").fadeIn("slow");

    }else{
        $.ajax({
           url: baseUrl+"youtube_trending.php",
                  type:"GET",
                  data:{
                       "country":country
                  },
           success: function(html) {
               paginator.save("youtube","youtube_trending",country,html);

               $("#trending_div .ui-loader-trending").hide();
               $.mobile.loading("hide");

               $("#trending").html(html);
               $("#trending").fadeIn("slow");

               $.mobile.activePage.find("#trending").trigger("create").fadeIn("slow");
           },
           error: function(){
               $("#trending_div .ui-loader-trending i").text("An error occured, Unable to connect to host.");
           }
       });
    }
}

$(document).on('click', '#trendingNavBtn', function() {
    showLoading();
    $("#trending").fadeOut("slow");
    var linkStr = $(this).attr("data-link");
    $.ajax({
        type: "GET",
        url: baseUrl+"youtube_trending.php",
        data: {
            nav : linkStr+"&country="+youtube.getCurrentCountry()
        },
        success: function(html) {
            $.mobile.loading("hide");
            $("#trending").html(html);
            $("#trending").fadeIn("slow");
            $.mobile.activePage.find("#trending").trigger("create").fadeIn("slow");
         }
        });
});


$(document).on('click', '#close_related', function() {
     showLoading();
    if($('#manul_youtube_page #trending').children().length > 0
            || $('#manul_youtube_page #result_div').children().length > 0){
        $.mobile.changePage($('#manul_youtube_page'));
    }else{
        mainwindow.browse_youtube();
    }
});

function getCountry(){
    $.get("https://ipinfo.io", function(response) {
        youtube.saveGeo(response.country);
    }, "jsonp");
}

function watch_video(track_id){

    var searchterm = $('#'+track_id).parent().attr("onclick").split("gettrackinfo(\"")[1].split(");")[0];
    var arr = searchterm.split("!=-=!")
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];
    showLoading();

    toDataUrl(coverUrl, function(myBase64) {
        document.querySelector("#coverImage").setAttribute("src",coverUrl);
        base64 = myBase64;
        var img = document.querySelector("#coverImage");
        if (!img.complete) {
          img.addEventListener('load', function handler(e) {
              e.currentTarget.removeEventListener(e.type, handler);
              dominantColor = colorThief.getColor(img);
              mainwindow.web_watch_video(songId+"<==>"+title+"<==>"+album+"<==>"+artist+"<==>"+coverUrl+"<==>"+songId+"<br>"+"<==>"+base64+"<==>"+dominantColor+"<==>"+artistId+"<==>"+albumId);
              $.mobile.loading("hide");
          });

        }

    });

}

function track_option(track_id){
    var channelHref = $('#'+track_id).parent().attr("data-channelhref");
    var searchterm = $('#'+track_id).parent().attr("onclick").split("gettrackinfo(\"")[1].split(");")[0];
    var arr = searchterm.split("!=-=!")
    title = arr[0];
    artist = arr[1];
    album = arr[2];
    coverUrl = arr[3];
    songId = arr[4];
    albumId = arr[5];
    artistId= arr[6];
    millis = arr[7];

    title = title.replace("\'"," ");

    //onclick=\''+$('#'+track_id).parent().attr("onclick")+'\'
    //https://www.youtube.com/watch?v=eqBkiu4M0Os
    var target = $( this ),
            options = '<hr><ul style="padding-bottom:5px" data-inset="true">'+
                        '<li>'+
                            '<a href="#" id="'+songId+'_addToQueue" >Add to queue</a>'+
                        '</li>'+
                        '<li>'+
                            '<a href="#" id="'+songId+'_watchVideo" onclick="watch_video(\''+songId+'\');$(\'#popup-'+songId +'\').remove();$(\'body\').css(\'overflow\',\'auto\');">Video Options</a>'+
                        '</li>'+
                        '<li>'+
                            '<a href="#" onclick="open_channel(\''+channelHref.trim()+'\', \''+songId+'\')" >Open Channel</a>'+
                        '</li>'+
                        '<li>'+
                            '<a href="#" onclick="show_related(\''+songId+'\',\''+title.replace("'"," ")+'\')" >Show Recommended Videos</a>'+
                        '</li>'+
                      '</ul>',
                link = "<span >id: "+ songId+"</span>",
                closebtn = '<a href="#" data-rel="back" class="ui-btn ui-corner-all ui-icon-delete ui-btn-icon-notext ui-btn-right">Close</a>',
                header = '<div style="margin: -12px -12px 0px -12px;" data-role="header"><h2>Options</h2></div>',
                img = '<img style="padding: 20px 0px 10px 0px;width:50% !important;height:50% !important;" src="'+coverUrl+'" alt="' + title + '" class="photo">',
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

        //scroll fix after popup init

        $( document ).on( "popupbeforeposition", $('#popup-'+songId ), function() {
            $( '#popup-'+songId).find("ul").listview();
            $("html").css("overflow-y","hidden");
        });

        // Remove the popup after it has been closed
        $( document ).on( "popupafterclose", $('#popup-'+songId), function() {
            $( '#popup-'+songId ).remove();
             $("html").css("overflow-y","scroll");
        });
}

function show_related(video_id,v_title){
    $("#popup-"+songId).remove();
    $('body').css('overflow','auto');
    showLoading();
    if(paginator.isOffline("youtube","show_related",video_id))
    {
        var html_ = paginator.load("youtube","show_related",video_id);
        $.mobile.changePage($('#manul_youtube_related_page'));
        $.mobile.loading("hide");
        $("#manul_youtube_related_page #result_div").html(html_);
        $('#manul_youtube_related_page .ui-content').trigger("create");
        $('#manul_youtube_related_page .ui-content').fadeIn('slow');

    }else{
        $.ajax({
            type: "GET",
            url: baseUrl+"youtube_related_videos.php",
            data: {
                "video_id" : video_id,
                "v_title"  : v_title
            },
            success: function(html) {
                paginator.save("youtube","show_related",video_id,html);
                $.mobile.changePage($('#manul_youtube_related_page'));
                $.mobile.loading("hide");
                $("#manul_youtube_related_page #result_div").html(html);
                $('#manul_youtube_related_page .ui-content').trigger("create");
                $('#manul_youtube_related_page .ui-content').fadeIn('slow');
             }
            });
    }
}

function get_channel(video_id){
    $('.ui-content').fadeOut('slow');
    showLoading();
    $.ajax({
        type: "GET",
        url: baseUrl+"youtube_get_channel.php",
        data: {
            "video_id" : video_id
        },
        success: function(html) {
            open_channel(html,video_id);
         }
        });
}

function open_channel(channelHref,songId){
    //$("#manul_youtube_page_suggestions").empty();
    $("#popup-"+songId).remove();
    $('body').css('overflow','auto');
    showLoading();
   //$('.ui-content').fadeOut('slow');
    $.ajax({
        type: "GET",
        url: baseUrl+"manual_youtube_search.php",
        data: {
            nav : channelHref
        },
        success: function(html) {
            $.mobile.changePage($('#manul_youtube_page'));
            $.mobile.loading("hide");
            $("#result_div").html(html);
            $('#manul_youtube_page .ui-content').trigger("create");
            $('#manul_youtube_page .ui-content').fadeIn('slow');
            $('#manul_youtube_page_suggestions').html("");
            $("#trending_div").hide();
            $("#history_div").hide();
            mainwindow.checkForPlaylist(); //special case cause we are loading results on same page
         }
        });
}
