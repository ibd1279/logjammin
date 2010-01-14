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
                $('<span/>').text(self.val()).insertAfter(self);
            });
        };
        $('input.tag-list').keypress(function(e) {
            if(e.which == 13) {
                var self = $(this);
                if(self.val().length < 1) return false;            
                self.siblings('ul.tag-list').append($('<li/>').append(self.clone(false)));
                toolbox_taglist_display(self.parent());
                self.val('');
                return false;
            }
            return true;
        });
        toolbox_taglist_display(selector);
    },
    redraw : function(selector) {
        this.taglist(selector);
    }
}

$(document).ready(function() { 
    Toolbox.redraw($("*"));
});
