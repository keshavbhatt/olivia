var baseUrl = "http://ktechpit.com/USS/Olivia/radio/"

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

var countries = [
            "Algeria",
            "Argentina",
            "Australia",
            "Austria",
            "Bangladesh",
            "Belarus",
            "Belgium",
            "Bosnia and Herzegovina",
            "Brasil",
            "Brazil",
            "Bulgaria",
            "Canada",
            "Chile",
            "China",
            "Colombia",
            "Costa Rica",
            "Croatia",
            "Cyprus",
            "Czech Republic",
            "Denmark",
            "Deutschland",
            "Dominican Republic",
            "Ecuador",
            "Egypt",
            "España",
            "Estonia",
            "Finland",
            "France",
            "Germany",
            "Greece",
            "Honduras",
            "Hungary",
            "Iceland",
            "India",
            "Indonesia",
            "Iran",
            "Iraq",
            "Ireland",
            "Israel",
            "Italy",
            "Jamaica",
            "Japan",
            "Kazakhstan",
            "Latvia",
            "Lebanon",
            "Lithuania",
            "Luxembourg",
            "Malta",
            "Mexico",
            "Moldova",
            "Morocco",
            "Netherlands",
            "Netherlands Antilles",
            "New Zealand",
            "Nigeria",
            "Norway",
            "Pakistan",
            "Peru",
            "Philippines",
            "playing",
            "Poland",
            "Portugal",
            "Puerto Rico",
            "Romania",
            "Russia",
            "Russian Federation",
            "Serbia",
            "Singapore",
            "Slovakia",
            "Slovenia",
            "South Africa",
            "South Korea",
            "Spain",
            "Sri Lanka",
            "Sweden",
            "Switzerland",
            "Syria",
            "Taiwan",
            "Thailand",
            "Trinidad and Tobago",
            "Tunisia",
            "Turkey",
            "UK",
            "Ukraine",
            "United Arab Emirates",
            "United Kingdom",
            "United States",
            "United States of America",
            "Uruguay",
            "USA",
            "Venezuela",
            "Vietnam"
        ]


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

var html_data; //global html_data for youtube search
var stations_loaded = false;
var favourite_loaded = false;


$(document).ready(function($) {

    $(function () {
        $("[data-role=popup]").popup().enhanceWithin();
    });

    $(function() {
           $.mobile.defaultHomeScroll = 0;
    });

});


//  core functions -------------

function loadCountries(){
    $.ajax({
        url: baseUrl+"list/countries.php",
               type:"GET",
        success: function(html) {
            $("#countries_result").append(html).listview("refresh");
        }
    });
}

function loadLanguages(){
    $.ajax({
        url: baseUrl+"list/languages.php",
               type:"GET",
        success: function(html) {
            $("#languages_result").append(html).listview("refresh");
        }
    });
}

function showStations(query,queryType){
    showLoading();
    //if offline data available
    if(paginator.isOffline("radio","showStations",query+"<==>"+queryType)){
        var html_ = paginator.load("radio","showStations",query+"<==>"+queryType);
        $.mobile.loading("hide");
        $.mobile.changePage($('#stations_page'));
        $("#stations_result").html(html_).listview("refresh");
        $('#stations_page .ui-content').trigger('create');
        $('#stations_page .ui-content').fadeIn('slow');
    }else{
        $.ajax({
           url: baseUrl+"list/stations.php",
                  type:"GET",
                   data:{
                       "query":escape(query),
                       "type":queryType
                   },
           success: function(html) {
               paginator.save("radio","showStations",query+"<==>"+queryType,html);
               $.mobile.loading("hide");
               $.mobile.changePage($('#stations_page'));
               $("#stations_result").html(html).listview("refresh");
               $('#stations_page .ui-content').trigger('create');
               $('#stations_page .ui-content').fadeIn('slow');
           }
       });
    }
}

function loadTopStations(type){
    if(paginator.isOffline("radio","loadTopStations",type)){
        if(type==="most-played"){
             var html_ = paginator.load("radio","loadTopStations",type);
             $("#most-played").html(html_).listview("refresh");
             $("#most-played-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
             $("#most-voted-tab-anchor").attr("style","");
        }
        if(type==="most-voted"){
             var html = paginator.load("radio","loadTopStations",type);
             $("#most-voted").html(html).listview("refresh");
             $("#most-voted-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
             $("#most-played-tab-anchor").attr("style","");
        }
    }else{
    //topStations
        $.ajax({
           url: baseUrl+"list/top-stations.php",
                  type:"GET",
                   data:{
                    "type":type
                   },
           success: function(html) {
               if(type==="most-played"){
                    paginator.save("radio","loadTopStations",type,html);
                    $("#most-played").html(html).listview("refresh");
                    $("#most-played-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;")
                    $("#most-voted-tab-anchor").attr("style","");
               }
               if(type==="most-voted"){
                    paginator.save("radio","loadTopStations",type,html);
                    $("#most-voted").html(html).listview("refresh");
                    $("#most-voted-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
                    $("#most-played-tab-anchor").attr("style","");
               }
           }
       });
    }
}

function playStation(streamDetail){
    mainwindow.playRadioFromWeb(streamDetail);
}

$(document).on("pagebeforeshow","#radio_page",function(){
    $('#radio_search_input').unbind();

    $('#radio_search_input').keypress(function(event){
        var keycode = (event.keyCode ? event.keyCode : event.which);
        if(keycode == "13" || keycode === "Enter"){ //ignore this warning
            station_search($(this).val())
        }
    });
});

function station_search(query){
    showLoading();
    if(paginator.isOffline("radio","station_search",query)){
        $.mobile.loading("hide");
        $.mobile.changePage($('#stations_page'));

        var html = paginator.load("radio","station_search",query);

        $("#stations_result").html(html).listview("refresh");
        $('#stations_page .ui-content').trigger('create');
        $('#stations_page .ui-content').fadeIn('slow');
    }else{
        $.ajax({
           url: baseUrl+"list/search.php",
                  type:"GET",
                   data:{
                    "query":query
                   },
           success: function(html) {
               $.mobile.loading("hide");
               $.mobile.changePage($('#stations_page'));

               paginator.save("radio","station_search",query,html);

               $("#stations_result").html(html).listview("refresh");
               $('#stations_page .ui-content').trigger('create');
               $('#stations_page .ui-content').fadeIn('slow');
           }
       });
    }
}



// open popup on channel_option is clicked
//playStation("112504=,=http://stream-uk1.radioparadise.com/aac-64=,=Radio Paradise=,=United States of America=,=english")
function channel_option(channel_id){
    var streamDetail = $('#'+channel_id).parent().attr("onclick").split("playStation(\"")[1].split(");")[0];
    var arr = streamDetail.split("=,=")
    var channelHref = arr[1];
    title = arr[2];
    album = arr[3]; //country
    artist = arr[4]; //lang
    coverUrl = "qrc:/web/radio/station_cover.jpg";
    songId = channel_id;

    //onclick=\''+$('#'+channel_id).parent().attr("onclick")+'\'
    //https://www.youtube.com/watch?v=eqBkiu4M0Os
    var channelSpecificOption;
    if($.mobile.activePage.attr("id") === "favourite_page"){
        channelSpecificOption = '<a href="#" id="'+songId+'_removeFavourite" >Remove from Favourite</a>';
    }else{
        channelSpecificOption = '<a href="#" id="'+songId+'_addFavourite" >Add to Favourite</a>';
    }
    var target = $( this ),
                options = '<hr><ul style="padding-bottom:5px" data-inset="true">'+
                        '<li>'+
                            '<a href="#" id="'+songId+'_playChannel" >Play Channel</a>'+
                        '</li>'+
                        '<li>'+
                            channelSpecificOption+
                        '</li>'+
                      '</ul>',
                link = "<span >id: "+ songId+"</span>",
                closebtn = '<a id="'+songId+'_closePopup" class="ui-btn ui-corner-all ui-icon-delete ui-btn-icon-notext ui-btn-right">Close</a>',
                header = '<div style="margin: -12px -12px 0px -12px;" data-role="header"><h2>Options</h2></div>',
                img = '<img style="padding: 20px 0px 10px 0px;" src="'+coverUrl+'" alt="' + title + '" class="photo">',
                details = $('#'+channel_id).parent().find("p")[0].outerHTML,
                popup = '<div data-history="false" style="text-align:center;padding:12px 12px; max-width:400px" data-transition="slideup" data-overlay-theme="b" data-dismissible="true" data-position-to="window" data-role="popup" id="popup-'+songId+'"  data-corners="false" data-tolerance="15"></div>';
            $( link ).appendTo($( details ));
            // Create the popup.
            $( header )
                .appendTo( $( popup )
                .appendTo( $.mobile.activePage )
                .popup() )
                .toolbar()
                .before( closebtn )
                .after( img + details + options);
                $( "#popup-" + songId ).find('p').attr('style',"");
                $( "#popup-" + songId ).find("ul").listview();
                $( "#popup-" + songId ).popup( "open" ).trigger("create");
                $('body').css('overflow','hidden');


        $("#"+songId+"_playChannel").on("click",function(){
                playStation(streamDetail);
                $('#popup-'+songId ).popup("close");
                $('body').css('overflow','auto');
        });

        $("#"+songId+"_addFavourite").on("click",function(){
                mainwindow.saveRadioChannelToFavourite(arr);
                $('#popup-'+songId ).popup("close");
                $('body').css('overflow','auto');
                favourite_loaded = false; //trigger refresh
        });

        $("#"+songId+"_removeFavourite").on("click",function(){
                store.removeRadioChannelFromFavourite(songId);
                $('#popup-'+songId ).popup("close");
                $('body').css('overflow','auto');
                favourite_loaded = false; //trigger refresh
                $('#favourite_page .ui-content').fadeOut();
                load_favourite();
                $('#favourite_page .ui-content').fadeIn();
        });

        $("#"+songId+"_closePopup").on("click",function(){
            $('#popup-'+songId ).popup("close");
            $('body').css('overflow','auto');
        });

        $( document ).on( "popupbeforeposition", $('#popup-'+songId ), function() {
            $('#popup-'+songId).find("ul").listview();
            $('body').css('overflow','hidden');
        });

        // Remove the popup after it has been closed
        $( document ).on( "popupafterclose", $('#popup-'+songId), function() {
            $('#popup-'+songId ).remove();
            $.mobile.loading("hide");
            $('body').css('overflow','auto');
        });
}


function open_radio_page(){
    $.mobile.changePage($('#radio_page'));
}

function open_favourite_page(){
    $.mobile.changePage($('#favourite_page'));
    if(!favourite_loaded){
        load_favourite();
    }
}

function load_favourite(){
    $("#fav_stations_result").empty();
    showLoading();
    var json = JSON.parse(store.web_print_fav_radio_channels()); // Data is returned in json format
    var $html = "";
    for(var i= 0; i < json.length;i++){
        var streamDetail = json[i].channelId+"=,="+json[i].url+"=,="+json[i].title+"=,="+json[i].country+"=,="+json[i].lang;
         $html = $html+
            "<li  data-filtertext='"+json[i].title+" "+json[i].country+" "+json[i].lang+"' >"+
                "<a onclick='playStation(\""+streamDetail+"\")'>"+
                "<img id='"+json[i].channelId+"' style='max-width:144px;max-height:142px;height:142px'  src='"+json[i].base64+"' \>"+
                        "<p style='line-height: 35px;'>"+
                            "Title: "+json[i].title+
                            "<br>"+
                            "Country: "+json[i].country+
                            "<br>"+
                            "Language: "+json[i].lang+
                        "</p>"+
                 "</a>"+
                 "<a href='#' onclick=\"channel_option(\'"+json[i].channelId+"\')\">More Options</a>"+
            "</li>";
    }
    $.mobile.activePage.find("#inner_header").html(json.length+" favourite radio stations");
    $.mobile.loading("hide");
    $("#fav_stations_result").append($html).listview("refresh");
    $('#favourite_page .ui-content').trigger('create');
    $('#favourite_page .ui-content').fadeIn('slow');
    favourite_loaded = true;
}


