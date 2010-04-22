// Toolbox for logjammin.
Toolbox = {
    taglist : function(selector) {
        var toolbox_taglist_display = function(root_selector) {
            root_selector.find('ul.tag-list li input:text:visible').each(function(i) {
                var self = $(this);
                $('<a href="#">X</a>').insertAfter(self).toggle(function() {
                    var parent = $(this).parent();
                    parent.children('span').css('text-decoration', 'line-through');
                    parent.children('input').attr('disabled', 'disabled');
                    return false;
                }, function() {
                    var parent = $(this).parent();
                    parent.children('span').css('text-decoration', '');
                    parent.children('input').removeAttr('disabled');
                    return false;
                });
                self.hide();
                if(self.attr("friendly")) {
                    $('<span/>').text(self.attr("friendly")).insertAfter(self);
                } else {
                    $('<span/>').text(self.val()).insertAfter(self);
                }
            });
        };
        $('input.tag-list').keypress(function(e) {
            if(e.which == 13) {
                var self = $(this);
                if(self.val().length < 1) return false;            
                self.siblings('ul.tag-list')
                    .append($('<li/>')
                    .append($('<input type="text" />')
                        .attr("name", self.attr("name"))
                        .val(self.val())
                    ));
                toolbox_taglist_display(self.closest('div'));
                self.val('');
                return false;
            }
            return true;
        });
        $('select.tag-list').change(function(e) {
            var self = $(this);
            if(self.val().length < 1) return false;
            self.siblings('ul.tag-list')
                .append($('<li/>')
                .append($('<input type="text" />')
                    .val(self.val())
                    .attr("name", self.attr("name"))
                    .attr("friendly", self.children('option:selected').text())
                ));
            toolbox_taglist_display(self.closest('div'));
            self.val('');
            return false;
        });
        toolbox_taglist_display(selector);
    },
    fetch : function(selector) {
        selector.find('a.fetch[href]').each(function(i) {
            var self = $(this);
            var div = $('<div/>');
            self.replaceWith(div.load(self.attr('href')));
            Toolbox.redraw(div);
        });
        selector.find('a.fetch-toggle[href]').each(function(i) {
            var self = $(this);
            self.one('click', function() {
                var div = $('<div />');
                div.hide().insertAfter(self).load(self.attr('href'), null, function() {
                    self.click(function() {
                        div.slideToggle();
                        return false;
                    });
                    self.click();
                    Toolbox.redraw(div);
                });
                return false;
            });
        });
    },
    redraw : function(selector) {
        this.taglist(selector);
        this.fetch(selector);
    }
}

$(document).ready(function() { 
    Toolbox.redraw($("*"));
});
