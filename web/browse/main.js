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

function showLoading() {
    $.mobile.loading("show", {
        text: "Loading..",
        textVisible: false,
        theme: "b",
        overlay: true
    });
}



//onclicks functions
$(document).on("click","#overview",function(){
    overview();
});


//  core functions -------------
function overview(){
    showLoading();
    $('#overview_page .ui-content').fadeOut('slow');
    $.ajax({
        url: baseUrl+"overview.php",
        success: function(html) {
            $.mobile.loading("hide");
            $('#overview_page .ui-content').fadeOut('slow');
            $('#overview_page .ui-content').html('');
            $('#overview_page .ui-content').html(html);
            $('#overview_page .ui-content').trigger('create').fadeIn('slow');
        }
    });
}

