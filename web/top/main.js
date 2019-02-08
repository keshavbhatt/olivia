var baseUrl = "http://ktechpit.com/USS/Olivia/"


var countries = {
    "Albania":"al",
    "Algeria":"dz",
    "Angola":"ao",
    "Anguilla":"ai",
    "Antigua and Barbuda":"ag",
    "Argentina":"ar",
    "Armenia":"am",
    "Australia":"au",
    "Austria":"at",
    "Azerbaijan":"az",
    "Bahamas":"bs",
    "Bahrain":"bh",
    "Barbados":"bb",
    "Belarus":"by",
    "Belgium":"be",
    "Belize":"bz",
    "Benin":"bj",
    "Bermuda":"bm",
    "Bhutan":"bt",
    "Bolivia":"bo",
    "Botswana":"bw",
    "Brazil":"br",
    "British Virgin Islands":"vg",
    "Brunei Darussalam":"bn",
    "Bulgaria":"bg",
    "Burkina Faso":"bf",
    "Cambodia":"kh",
    "Canada":"ca",
    "Cape Verde":"cv",
    "Cayman Islands":"ky",
    "Chad":"td",
    "Chile":"cl",
    "China":"cn",
    "Colombia":"co",
    "Congo, Republic of the":"cg",
    "Costa Rica":"cr",
    "Croatia":"hr",
    "Cyprus":"cy",
    "Czech Republic":"cz",
    "Denmark":"dk",
    "Dominica":"dm",
    "Dominican Republic":"do",
    "Ecuador":"ec",
    "Egypt":"eg",
    "El Salvador":"sv",
    "Estonia":"ee",
    "Fiji":"fj",
    "Finland":"fi",
    "France":"fr",
    "Gambia":"gm",
    "Germany":"de",
    "Ghana":"gh",
    "Greece":"gr",
    "Grenada":"gd",
    "Guatemala":"gt",
    "Guinea-Bissau":"gw",
    "Guyana":"gy",
    "Honduras":"hn",
    "Hong Kong":"hk",
    "Hungary":"hu",
    "Iceland":"is",
    "India":"in",
    "Indonesia":"id",
    "Ireland":"ie",
    "Israel":"il",
    "Italy":"it",
    "Jamaica":"jm",
    "Japan":"jp",
    "Jordan":"jo",
    "Kazakhstan":"kz",
    "Kenya":"ke",
    "Korea, Republic Of":"kr",
    "Kuwait":"kw",
    "Kyrgyzstan":"kg",
    "Lao, People's Democratic Republic":"la",
    "Latvia":"lv",
    "Lebanon":"lb",
    "Liberia":"lr",
    "Lithuania":"lt",
    "Luxembourg":"lu",
    "Macau":"mo",
    "Macedonia":"mk",
    "Madagascar":"mg",
    "Malawi":"mw",
    "Malaysia":"my",
    "Mali":"ml",
    "Malta":"mt",
    "Mauritania":"mr",
    "Mauritius":"mu",
    "Mexico":"mx",
    "Micronesia, Federated States of":"fm",
    "Moldova":"md",
    "Mongolia":"mn",
    "Montserrat":"ms",
    "Mozambique":"mz",
    "Namibia":"na",
    "Nepal":"np",
    "Netherlands":"nl",
    "New Zealand":"nz",
    "Nicaragua":"ni",
    "Niger":"ne",
    "Nigeria":"ng",
    "Norway":"no",
    "Oman":"om",
    "Pakistan":"pk",
    "Palau":"pw",
    "Panama":"pa",
    "Papua New Guinea":"pg",
    "Paraguay":"py",
    "Peru":"pe",
    "Philippines":"ph",
    "Poland":"pl",
    "Portugal":"pt",
    "Qatar":"qa",
    "Romania":"ro",
    "Russia":"ru",
    "Saudi Arabia":"sa",
    "Senegal":"sn",
    "Seychelles":"sc",
    "Sierra Leone":"sl",
    "Singapore":"sg",
    "Slovakia":"sk",
    "Slovenia":"si",
    "Solomon Islands":"sb",
    "South Africa":"za",
    "Spain":"es",
    "Sri Lanka":"lk",
    "St. Kitts and Nevis":"kn",
    "St. Lucia":"lc",
    "St. Vincent and The Grenadines":"vc",
    "Suriname":"sr",
    "Swaziland":"sz",
    "Sweden":"se",
    "Switzerland":"ch",
    "São Tomé and Príncipe":"st",
    "Taiwan":"tw",
    "Tajikistan":"tj",
    "Tanzania":"tz",
    "Thailand":"th",
    "Trinidad and Tobago":"tt",
    "Tunisia":"tn",
    "Turkey":"tr",
    "Turkmenistan":"tm",
    "Turks and Caicos":"tc",
    "Uganda":"ug",
    "Ukraine":"ua",
    "United Arab Emirates":"ae",
    "United Kingdom":"gb",
    "United States":"us",
    "Uruguay":"uy",
    "Uzbekistan":"uz",
    "Venezuela":"ve",
    "Vietnam":"vn",
    "Yemen":"ye",
    "Zimbabwe":"zw"
}


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



function get_top_tracks(country){
     showLoading();
     $.ajax({
        url: baseUrl+"top_lists.php",
               type:"GET",
               data:{
                    "con":country
               },
        success: function(html) {
            $.mobile.loading("hide");
            $.mobile.changePage($('#track_result_view_page'));
            $("#track_result_view_page .ui-content").html(html);
            $('#track_result_view_page .ui-content').trigger('create');
            $('#track_result_view_page .ui-content').fadeIn('slow');
        }
    });
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

function setNowPlaying(songId){ //nowPlaying styles are in main.css

    //removes all now playing

    $(".nowPlaying").remove();
    $.mobile.activePage.remove(".nowPlaying");

    //adds nowPlaying pages
    $("#"+songId).css("cssText","position: absolute;left: 0px;top: 0px;");
    $("#"+songId).each(function( index ) {
      $( this ).before("<div class='nowPlaying'></div>");
    });

    //for album [adds nowPlaying active page]
    $.mobile.activePage.find("#"+songId).css("cssText","position: absolute;left: 0px;top: 0px;");
    $.mobile.activePage.find("#"+songId).before("<div class='nowPlaying'></div>");

}



