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
            if($(html).text().indexOf("Olivia suggest")>0){
                $('#tracks_page .ui-content').html(html);
            }else{
                $("#tracks_result").append(html).listview("refresh");
            }
            $('#tracks_page .ui-content').trigger('create');
            $('#tracks_page .ui-content').fadeIn('slow');
            track_loaded = true;
            mainwindow.resultLoaded();
        }
    });
}


function manual_youtube_search(term){
    $.mobile.changePage($('#manul_youtube_page'));
    if(term===""){
        term = $("#manual_search").val();
    }
    showLoading();
    $("#result_div").html("");
    $.ajax({
       url: baseUrl+"manual_youtube_search.php",
              type:"GET",
              data:{
                   "query":term
              },
       success: function(html) {
           $.mobile.loading("hide");
           $("#result_div").html(html);
           $('#manul_youtube_page .ui-content').trigger("create");
           $('#manul_youtube_page .ui-content').fadeIn('slow');
           $('#manul_youtube_page_suggestions').html("");
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
     $.ajax({
        url: baseUrl+"album_view.php",
               type:"GET",
               data:{
                    "query":id
               },
        success: function(html) {
            $.mobile.loading("hide");
            $.mobile.changePage($('#album_view_page'));
            $("#album_view_page .ui-content").html(html);
            $('#album_view_page .ui-content').trigger('create');
            $('#album_view_page .ui-content').fadeIn('slow');
        }
    });
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

function setManualSearchVal(text){
    $("#manual_search").val(text);
    $( "#manul_youtube_page_suggestions" ).empty();
}



$(document).on("pagebeforeshow","#manul_youtube_page",function(){
    $('#manual_search').unbind();

    $('#manual_search').keypress(function(event){
        var keycode = (event.keyCode ? event.keyCode : event.which);
        if(keycode == '13'){
            manual_youtube_search($(this).val());
        }
    });
});




$( document ).on( "pagecreate", "#manul_youtube_page", function() {
    $( "#manul_youtube_page_suggestions" ).on( "filterablebeforefilter", function ( e, data ) {
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
                        html += '<li onclick="setManualSearchVal(\''+term+'\');" >'+term+'</li>';
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
    $('#result_div').html("");
    $.ajax({
        type: "GET",
        url: baseUrl+"manual_youtube_search.php",
        data: {
            nav : linkStr
        },
        success: function(html) {
            $.mobile.loading("hide");
            $("#result_div").html(html);
            $('#manul_youtube_page .ui-content').trigger("create");
            $('#manul_youtube_page .ui-content').fadeIn('slow');
            $('#manul_youtube_page_suggestions').html("");
         }
        });
});

function orderChanged(linkStr,pageType){
    showLoading();
    $('#result_div').html("");
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
            $('#manul_youtube_page .ui-content').trigger("create");
            $('#manul_youtube_page .ui-content').fadeIn('slow');
            $('#manul_youtube_page_suggestions').html("");
         }
        });
}


