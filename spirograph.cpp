#include "spirograph.hpp"

int main(int argc, char **argv)
{
    initialize_SDL();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Create root node
    Spirograph spirograph_base_node({display.width / 2.0f, display.height / 2.0f}, {0, 0.1});
    spirograph_base_node.revps = 0;
    spirograph_base_node.is_root = true;

    // Initialize mode and timer
    enum Mode mode = EDIT;
    auto startTime = std::chrono::steady_clock::now();

    // * Main loop
    bool running = true;
    while (running)
    {
        // Handle events, update keyboard states and clear the renderer
        handleEvents(&running, &mode, &spirograph_base_node);
        keyboardState.keystates = SDL_GetKeyboardState(NULL); // Update keyboard states
        clearRenderer();

        // Calculate dt
        auto currentTime = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(currentTime - startTime).count();
        startTime = currentTime;

        // * Modes
        switch (mode) {
        case EDIT:
            edit(&spirograph_base_node, dt);

            // Change mode to ANIMATE on space
            if (keyboardState.keydown(SDL_SCANCODE_SPACE) && editorState.edit_mode == EditorState::EDIT_MENU) 
            {
                mode = ANIMATE;
                spirograph_base_node.reset();
                spirograph_base_node.update_trail_first_point();
            }
            break;

        case ANIMATE:
            editorState.edit_mode = EditorState::EDIT_MENU;
            spirograph_base_node.draw_trail();
            if (play) 
            {   // Draw vectors and rotate when the animation is not paused
                spirograph_base_node.rotate(dt);
                spirograph_base_node.draw(Spirograph::HIGHLIGHT);
            }

            // Keyboard events to pause/unpause on space and change modes on r
            if (keyboardState.keydown(SDL_SCANCODE_SPACE))
            {   // Pause/Unpause
                play = !play;
            }
            else if (keyboardState.keydown(SDL_SCANCODE_R)) 
            {   // Change mode to EDIT
                mode = EDIT;
                play = true;
                spirograph_base_node.reset();
            }
            
            break; 
        }

        SDL_RenderPresent(renderer);
    }

    spirograph_base_node.free_members();
    quit_SDL();
    return 0;
}

// * Edit function definitions
void edit(Spirograph *spirograph_base_node, double dt)
{
    static Spirograph *selected_node = NULL, *closest_node = NULL, *new_child = NULL;
    spirograph_base_node->draw(Spirograph::UNHIGHLIGHT);

    if (editorState.edit_mode == EditorState::EDIT_MENU) // * Edit the selected node
    {
        // Editing functions and editing states
        bool editing_colour = false, editing_dirpos = false, editing_revps = false;
        colour_palette(selected_node, &editing_colour);
        change_rotation_speed(selected_node, &editing_revps, dt);
        if (!editing_colour && !editing_revps)
        {   // Editing states are used to prevent interference and only allow one thing to be edited at a time
            edit_dirpos(selected_node, &editing_dirpos);
            reselect_node(spirograph_base_node, closest_node, &selected_node, editing_dirpos);
        }

        // Toggle trail on key down
        if (keyboardState.keydown(selected_node->toggle_trail_key))
        {
            selected_node->trail_on = !(selected_node->trail_on);
        }

        // Delete node on key down
        if (keyboardState.keydown(selected_node->delete_node_key) && selected_node != spirograph_base_node)
        {
            Spirograph *old_parent = selected_node->parent;
            old_parent->remove_child(selected_node);
            selected_node = old_parent;

            if (selected_node->is_root && spirograph_base_node->children_length == 0)
            {   // Create new root after an old root was deleted
                editorState.creating_first = true;
                editorState.edit_mode = EditorState::SET_CHILD_POSITION;
            }
        }

        // Reset spirograph
        if (keyboardState.keydown(selected_node->reset_key) && !editorState.creating_first && spirograph_base_node->children_length > 0)
        {
            // Remove all the roots from the base node and set the selected node to the base node
            spirograph_base_node->clear_children();
            selected_node = spirograph_base_node;

            // Create a new root after old root was deleted
            editorState.creating_first = true;
            editorState.edit_mode = EditorState::SET_CHILD_POSITION;
        }

        // Todo: test
        // Switch modes when holding left CTRL
        if (keyboardState.keystates[SDL_SCANCODE_LCTRL])
        {
            editorState.edit_mode = EditorState::SET_CHILD_POSITION;
        }
    }
    else if (editorState.edit_mode == EditorState::SET_CHILD_POSITION) // * Set position of new node
    {
        // Get closest node to cursor
        float distance;
        Vec2Float orthproj;
        node_near_cursor_orthproj(spirograph_base_node, &selected_node, &distance, &orthproj);

        // Don't consider the base node
        if (selected_node == spirograph_base_node && spirograph_base_node->children_length > 0)
        {
            selected_node = spirograph_base_node->children[0];
        }
        Vec2Float new_pos = {selected_node->position_initial.x + orthproj.x, selected_node->position_initial.y + orthproj.y};

        // Draw segments
        selected_node->draw_direction(Spirograph::HIGHLIGHT);
        selected_node->draw_head(Spirograph::HIGHLIGHT);
        SDL_SetRenderDrawColor(renderer, YELLOW);
        SDL_RenderFillCircle(renderer, new_pos.x, new_pos.y, 3);
        drawLine(renderer, {ORANGE}, MouseState.pos.x, MouseState.pos.y, new_pos.x, new_pos.y);

        // Mouse events
        if (MouseState.left_down)
        {   // Add new child and switch to set direction mode
            new_child = new Spirograph(new_pos, {(float)MouseState.pos.x, (float)MouseState.pos.y});
            selected_node->add_child(new_child);
            new_child->direction_initial = new_child->direction = {MouseState.pos.x - new_child->position_initial.x, MouseState.pos.y - new_child->position_initial.y};
            editorState.edit_mode = EditorState::SET_CHILD_DIRECTION;

            if (selected_node == spirograph_base_node)
            {
                editorState.creating_first = false;
            }
        }

        // Todo: test
        // Switch modes when left CTRL up
        if (keyboardState.keyup(SDL_SCANCODE_LCTRL))
        {
            editorState.edit_mode = EditorState::EDIT_MENU;
        }
    }
    else if (editorState.edit_mode == EditorState::SET_CHILD_DIRECTION) // * Set direction of new node
    {
        // Fix direction vector to cursor and draw
        new_child->direction_initial = new_child->direction = {MouseState.pos.x - new_child->position_initial.x, MouseState.pos.y - new_child->position_initial.y};
        selected_node->draw_direction(Spirograph::HIGHLIGHT);
        selected_node->draw_head(Spirograph::HIGHLIGHT);

        // Mouse events
        if (MouseState.left_up)
        {
            MouseState.left_up = false;
            selected_node = new_child;
            if (!new_child->is_root)
            {
                new_child->trail_on = true;
            }

            // Change mode
            if (keyboardState.keystates[SDL_SCANCODE_LCTRL])
            {
                editorState.edit_mode = EditorState::SET_CHILD_POSITION;
            }
            else
            {
                editorState.edit_mode = EditorState::EDIT_MENU;
            }
        }
        else if (MouseState.right_down)
        {
            MouseState.right_down = false;
            editorState.edit_mode = EditorState::SET_CHILD_POSITION;
        }
    }

    return;
}

void reselect_node(Spirograph *spirograph_base_node, Spirograph *closest_node, Spirograph **selected_node, bool editing_dirpos)
{
    // Get closest node to cursor
    float node_distance;
    Vec2Float orthproj;
    node_near_cursor_orthproj(spirograph_base_node, &closest_node, &node_distance, &orthproj);

    // Don't consider the base node
    if (closest_node == spirograph_base_node)
    {
        if (spirograph_base_node->children_length > 0)
        {
            closest_node = spirograph_base_node->children[0];
        }
        else
        {
            editorState.edit_mode = EditorState::SET_CHILD_POSITION;
            return;
        }
    }

    // Draw closest and selected node
    if (!editing_dirpos)
    {
        closest_node->draw_direction(Spirograph::LIGHT_HIGHLIGHT);
        closest_node->draw_head(Spirograph::UNHIGHLIGHT);
    }
    (*selected_node)->draw_direction(Spirograph::HIGHLIGHT);
    (*selected_node)->draw_head(Spirograph::HIGHLIGHT);
    (*selected_node)->draw_base(Spirograph::HIGHLIGHT);
    
    // Mouse events
    if (MouseState.left_down && !editing_dirpos)
    {   // Select closest node on click
        MouseState.left_down = false;
        *selected_node = closest_node;
    }

    return;
}

void edit_dirpos(Spirograph *selected_node, bool *editing)
{
    // Holding and hovering base and head states
    static bool holding_head, holding_base;
    Vec2Float head_position = {selected_node->position_initial.x + selected_node->direction_initial.x, selected_node->position_initial.y + selected_node->direction_initial.y};
    const bool hovering_base = pow(MouseState.pos.x - selected_node->position_initial.x, 2) + pow(MouseState.pos.y - selected_node->position_initial.y, 2) <= pow(selected_node->hover_radius, 2);
    const bool hovering_head = pow(MouseState.pos.x - head_position.x, 2) + pow(MouseState.pos.y - head_position.y, 2) <= pow(selected_node->hover_radius, 2);

    // Update holding and hovering base and head states on mouse and keyboard events        
    if (keyboardState.keystates[selected_node->change_head_key] || (hovering_head && MouseState.left_down))
    {
        holding_head = true;
    }
    else if (!MouseState.left_down)
    {
        holding_head = false;
    }
    
    if (keyboardState.keystates[selected_node->change_base_key] || (hovering_base && !selected_node->is_root && MouseState.left_down))
    {
        holding_base = true;
    }
    else if (!MouseState.left_down)
    {
        holding_base = false;
    }

    // Change direction based on head state
    if (!hovering_head && !holding_head)
    {
        selected_node->head_radius = selected_node->default_radius;
    }
    else if (hovering_head)
    {   // Highlight the head when it's being hovered by increasing the radius
        selected_node->head_radius = selected_node->hover_radius;
        selected_node->draw_head(Spirograph::HIGHLIGHT);
    }
    else if (holding_head)
    {   // Fix the direction vector on mouse position
        selected_node->direction_initial = selected_node->direction = {MouseState.pos.x - selected_node->position_initial.x, MouseState.pos.y - selected_node->position_initial.y};
    }
    
    // Change position based on base state
    if (!hovering_base && !holding_base)
    {
        selected_node->base_radius = selected_node->default_radius;
    }
    else if (hovering_base && !selected_node->is_root)
    {   // Highlight the base when it's being hovered by increasing the radius
        selected_node->base_radius = selected_node->hover_radius;
        selected_node->draw_base(Spirograph::HIGHLIGHT);
    }
    else if (holding_base)
    {   // Fix the position vector on the orthogonal porjection of the mouse position on the parent direction
        // Get parent orthogonal projection taking direction vector boundaries into account
        Vec2Float parent_orthproj = selected_node->parent->get_cursor_orthogonalProjection();
        float proj_scale = parent_orthproj.x ? parent_orthproj.x / selected_node->parent->direction_initial.x : parent_orthproj.y / selected_node->parent->direction_initial.y;
        if (proj_scale <= 0) 
        {
            proj_scale = 0;
            parent_orthproj = {0, 0};
        }
        else if (proj_scale >= 1)
        {
            proj_scale = 1;
            parent_orthproj = selected_node->parent->direction_initial;
        }
        
        // Set new position and recalculate positionon parent
        selected_node->position_initial = selected_node->position = {selected_node->parent->position_initial.x + parent_orthproj.x, selected_node->parent->position_initial.y + parent_orthproj.y};
        selected_node->position_on_parent = proj_scale;
    }

    // Update children's position based on the new direction
    if (holding_head || holding_base)
    {
        selected_node->update_childrens_position_on_parent();
    }

    *editing = (hovering_base || holding_base || hovering_head || holding_head);

    return;
}

void node_near_cursor_orthproj(Spirograph *spirograph_base_node, Spirograph **closest, float *distance2, Vec2Float *orthproj)
{
    *closest = spirograph_base_node;

    // Calculate orthogonal projection distance from cursor
    *orthproj = spirograph_base_node->get_cursor_orthogonalProjection();
    float proj_scale = orthproj->x ? orthproj->x / spirograph_base_node->direction_initial.x : orthproj->y / spirograph_base_node->direction_initial.y;

    // Bounds for the orthogonal projection
    if (proj_scale <= 0)
    {
        *orthproj = {0, 0};
    }
    else if (proj_scale >= 1)
    {
        *orthproj = spirograph_base_node->direction_initial;
    }

    int mouseX = MouseState.pos.x, mouseY = MouseState.pos.y;
    float closest_distance2 = pow(spirograph_base_node->position_initial.x + orthproj->x - mouseX, 2) + pow(spirograph_base_node->position_initial.y + orthproj->y - mouseY, 2);

    // Find closest distance between parent and children
    for (int i = 0; i < spirograph_base_node->children_length; i++)
    {
        // Get closest grandchild
        Spirograph *closest_grandchild;
        float child_distance2;
        Vec2Float child_orthproj;
        node_near_cursor_orthproj(spirograph_base_node->children[i], &closest_grandchild, &child_distance2, &child_orthproj);

        // Check if its the minimum distance
        if (child_distance2 < closest_distance2)
        {
            closest_distance2 = child_distance2;
            *closest = closest_grandchild;
            *orthproj = child_orthproj;
        }
    }

    *distance2 = closest_distance2;

    return;
}

void colour_palette(Spirograph *current_node, bool *hovering)
{
    RGBA selected_colour = current_node->trail->colour;
    
    // Dimensions
    int partitions = 3;
    int margin = 20;
    int spacing = margin * 0.25;
    int button_radius = ((display.height - 2*margin) / (float)(partitions*partitions) - spacing*((partitions*partitions - 1) / (float)(partitions*partitions))) / 2.0f;
    int draw_distance = 2*button_radius + spacing;

    // Tell edit function if the user is hovering the colour palette interface
    *hovering = MouseState.pos.x <= 2*margin + (partitions-1)*spacing + partitions*2*button_radius;

    // Draw
    for (int ri = 0; ri < partitions; ri++)
    {
        int button_count = 0;
        for (int gi = 0; gi < partitions; gi++)
        {
            for (int bi = 0; bi < partitions; bi++, button_count++)
            {
                // Button properties
                RGBA button_colour = {ri * (255 / (float)(partitions - 1)), gi * (255 / (float)(partitions - 1)), bi * (255 / (float)(partitions - 1)), 255};
                int x = margin + button_radius + (ri * draw_distance);   
                int y = margin + button_radius + (button_count * draw_distance);   

                // Detect click
                bool hovering_button = pow(MouseState.pos.x - x, 2) + pow(MouseState.pos.y - y, 2) <= pow(button_radius, 2);
                if (MouseState.left_down && hovering_button) selected_colour = button_colour;

                // Highlight button when already selected
                if (button_colour.r == selected_colour.r && button_colour.g == selected_colour.g && button_colour.b == selected_colour.b && button_colour.a == selected_colour.a)
                {
                    SDL_SetRenderDrawColor(renderer, WHITE);
                    SDL_RenderFillCircle(renderer, x, y, button_radius + 0.45*spacing); 
                    SDL_SetRenderDrawColor(renderer, BLACK);
                    SDL_RenderFillCircle(renderer, x, y, button_radius + 0.15*spacing); 
                }
                else if (hovering_button)
                {
                    SDL_SetRenderDrawColor(renderer, WHITE);
                    SDL_RenderFillCircle(renderer, x, y, button_radius + 0.25*spacing); 
                    SDL_SetRenderDrawColor(renderer, BLACK);
                    SDL_RenderFillCircle(renderer, x, y, button_radius + 0.15*spacing); 
                }
                
                // Draw button 
                SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(button_colour));
                SDL_RenderFillCircle(renderer, x, y, button_radius); 
            }
        }
    }

    current_node->trail->colour = selected_colour;

    return;
}

void change_rotation_speed(Spirograph *selected_node, bool *editing, double dt)
{
    // Define static slider struct
    static struct
    {
        int margin = 30;
        int x = display.width - margin;
        int y;
        int top = margin;
        int bottom = display.height - margin;
        int r = 5;
        int hover_r = 7;
        float min_value = 0;
        float max_value = 2;
        float value;
    }
    slider;

    // Update value and y position based on the revolutions per second set on the selected node
    slider.value = selected_node->revps;
    slider.y = ((slider.top - slider.bottom) / (float)(slider.max_value - slider.min_value)) * slider.value + slider.bottom;

    // Detect slider and area hover
    bool hovering_slider = pow(MouseState.pos.x - slider.x, 2) + pow(MouseState.pos.y - slider.y, 2) <= pow(slider.hover_r, 2);
    bool hovering_area = MouseState.pos.x > display.width - 2*slider.margin;
    static bool holding = false;

    // Detect mouse clicks
    if (MouseState.left_down && hovering_area) holding = true;
    else if (!MouseState.left_down) holding = false;
    if (MouseState.left_up) selected_node->reset();

    // Update the edit function if actions are being performed on the slider
    *editing = hovering_slider || holding || hovering_area;

    // Draw slider
    SDL_SetRenderDrawColor(renderer, WHITE);
    SDL_RenderFillCircle(renderer, slider.x, slider.y, hovering_slider ? slider.hover_r : slider.r);
    drawLine(renderer, {WHITE}, slider.x, slider.top, slider.x, slider.bottom);

    // Control with the mouse if holding and update the rotation speed of the selected node based on the y position of the slider
    if (holding)
    {
        slider.y = MouseState.pos.y;
        if (slider.y > slider.bottom) slider.y = slider.bottom;
        if (slider.y < slider.top) slider.y = slider.top;

        selected_node->rotate(dt);
        
        selected_node->revps = ((slider.max_value - slider.min_value) / (float)(slider.top - slider.bottom)) * (slider.y - slider.bottom);
        selected_node->revps = ceil(selected_node->revps * 100) / 100.0f;
    }

    return;
}

// * SpirographNode method definitions
Spirograph::Spirograph(Vec2Float position_i, Vec2Float direction_i)
{
    // Initial position and direction vectors
    position_initial = position = position_i;
    direction_initial = direction = direction_i;

    head_radius = base_radius = default_radius;
    revps = 0.3;
    
    children_length = 0;
    children = (Spirograph**)malloc(0);
    parent = NULL;
    
    is_root = false;

    // Trail
    trail_on = false;
    trail = new Trail({WHITE});
    update_trail_first_point();
}

void Spirograph::rotate(double dt)
{
    // Cosine and sine of the angle adjusted by dt
    float cos_a = cos((revps * 2 * PI) * dt);
    float sin_a = sin((revps * 2 * PI) * dt);

    // Rotate direction vector
    direction = {(direction.x * cos_a) - (direction.y * sin_a), (direction.x * sin_a) + (direction.y * cos_a)};

    // Add points to trail
    if (play) trail->new_point({position.x + direction.x, position.y + direction.y});

    // Adjust children's positions based on rotation
    for (int i = 0; i < children_length; i++)
    {
        children[i]->position.x = position.x + (direction.x * children[i]->position_on_parent);
        children[i]->position.y = position.y + (direction.y * children[i]->position_on_parent);
    }

    // Rotate children
    for (int i = 0; i < children_length; i++)
    {
        children[i]->rotate(dt);
    }

    return;
}

void Spirograph::draw_trail()
{
    if (trail_on)
        trail->draw();
    for (int i = 0; i < children_length; i++)
        children[i]->draw_trail();
    return;
}

void Spirograph::update_trail_first_point()
{
    trail->first_point = {
        position_initial.x + direction_initial.x,
        position_initial.y + direction_initial.y
    };
    return;
}

void Spirograph::draw(HighlightType highlight_type)
{
    for (int i = 0; i < children_length; i++) 
    {
        children[i]->draw(highlight_type);
    }
    
    draw_direction(highlight_type);
    draw_head(highlight_type);

    return;
}

void Spirograph::draw_direction(HighlightType highlight_type)
{
    if (is_root) return;
    drawLine(renderer, highlightColour[highlight_type], position.x, position.y, position.x + direction.x, position.y + direction.y);
    return;
}

void Spirograph::draw_head(HighlightType highlight_type)
{
    RGBA colour = trail->colour;
    colour.a = highlightAlpha[highlight_type];
    
    if (is_root) return;

    if (trail_on)
    {
        SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(colour));
        SDL_RenderFillCircle(renderer, position.x + direction.x, position.y + direction.y, head_radius);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(display.background_colour));
        SDL_RenderFillCircle(renderer, position.x + direction.x, position.y + direction.y, head_radius);
        
        SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(colour));
        SDL_RenderDrawCircle(renderer, position.x + direction.x, position.y + direction.y, head_radius);
    }
    return;
}

void Spirograph::draw_base(HighlightType highlight_type)
{
    if (is_root) return;

    SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(display.background_colour));
    SDL_RenderFillCircle(renderer, position.x, position.y, base_radius);
    
    SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(highlightColour[highlight_type]));
    SDL_RenderDrawCircle(renderer, position.x, position.y, base_radius);

    return;
}

void Spirograph::reset()
{
    // Set position back to initial position
    position = position_initial;
    direction = direction_initial;

    // Reset trail
    trail->reset();

    // reset children
    for (int i = 0; i < children_length; i++)
    {
        children[i]->reset();
    }

    return;
}

void Spirograph::update_childrens_position_on_parent()
{
    // Update children's position on parent of this node
    for (int i = 0; i < children_length; i++)
    {
        children[i]->position_initial = children[i]->position = {
            position.x + (direction.x * children[i]->position_on_parent),
            position.y + (direction.y * children[i]->position_on_parent)
        };
        
        // Update children's position on parent of each of this node's children's children
        children[i]->update_childrens_position_on_parent();
    }

    return;
}

void Spirograph::add_child(Spirograph *new_child_ptr)
{
    // Append to dynamically allocated array of child pointers
    children_length++;

    children = (Spirograph**)realloc(children, sizeof(Spirograph*) * children_length);
    if (children == NULL)
    {
        printf("Failed to allocate memory to children array in Spiroraph\n");
        exit(1);
    }
    children[children_length - 1] = new_child_ptr;

    // Initialize child's parent pointer, position on parent, and rotation
    new_child_ptr->parent = this;
    Vec2Float child_end = {new_child_ptr->position_initial.x - position_initial.x, new_child_ptr->position_initial.y - position_initial.y};
    new_child_ptr->position_on_parent = (direction_initial.x) ? child_end.x / direction_initial.x : child_end.y / direction_initial.y;
    new_child_ptr->revps += revps;

    return;
}

void Spirograph::remove_child(Spirograph *old_child_ptr)
{
    if (children_length == 0) return;

    // Remove last child
    if (children[children_length - 1] == old_child_ptr)
    {
        free(children[children_length - 1]);
        children_length--;
        return;
    }

    // Remove first child
    if (children[0] == old_child_ptr)
    {
        free(children[0]);
        for (int i = 0; i < children_length - 1; i++)
        {
            children[i] = children[i + 1];
        }

        children_length--;
        children = (Spirograph**)realloc(children, sizeof(Spirograph*) * children_length);

        return;
    }

    // Remove an inner child
    for (int i = 0; i < children_length; i++)
    {
        if (children[i] == old_child_ptr)
        {
            for (int j = i; j < children_length - 1; j++)
            {
                children[j] = children[j + 1];
            }

            children_length--;
            children = (Spirograph**)realloc(children, sizeof(Spirograph*) * children_length);

            return;
        }
    }

    return;
}

void Spirograph::free_members()
{
    // Children
    for (int i = 0; i < children_length; i++)
    {
        children[i]->free_members();
    }
    free(children);

    return;
}

void Spirograph::clear_children()
{
    for (int i = 0; i < children_length; i++)
    {
        children[i]->clear_children();
        remove_child(children[i]);
    }
    children_length = 0;
    
    return;
}

Vec2Float Spirograph::get_cursor_orthogonalProjection()
{
    float proj_coeff =
        (((MouseState.pos.x - position_initial.x)*direction_initial.x) +
        ((MouseState.pos.y - position_initial.y)*direction_initial.y)) /
        (pow(direction_initial.x, 2) + pow(direction_initial.y, 2));
    Vec2Float orthogonal_projection = {proj_coeff * direction_initial.x, proj_coeff * direction_initial.y};
    return orthogonal_projection;
}

// * Trail method definitions
Trail::Trail(RGBA rgba)
{
    colour = rgba;
    first_point = current_point = previous_point = {0, 0};
    length = 0;
}

void Trail::draw()
{
    // Set target to trail texture and draw trail
    if (length >= 2)
    {
        SDL_SetRenderTarget(renderer, trail_texture);
        drawLine(renderer, colour, previous_point.x, previous_point.y, current_point.x, current_point.y);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, trail_texture, NULL, NULL);
    }

    return;
}

void Trail::new_point(Vec2Float point0)
{
    length++;
    previous_point = current_point;
    current_point = point0;
    return;
}

void Trail::reset()
{
    length = 0;
    SDL_SetRenderTarget(renderer, trail_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    return;
}

// * Vec2 Struct method definitions and operator overloads
template <typename T>
float Vec2<T>::length()
{
    return sqrt(x*x + y*y);
}

// * Colour functions and operator overloads
HSVA rgba_to_hsva(RGBA in)
{
    HSVA out;
    double min, max, delta;

    in = {in.r / 255.0f, in.g / 255.0f, in.b / 255.0f, in.a / 255.0f};
    out.a = in.a;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max; // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }

    if (max > 0.0)
    {   // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    }
    else
    {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }

    if (in.r >= max)                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else if (in.g >= max)
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if (out.h < 0.0)
        out.h += 360.0;

    return out;
}

RGBA hsva_to_rgba(HSVA in)
{
    double hh, p, q, t, ff;
    long i;
    RGBA out;

    out.a = in.a;

    if (in.s <= 0.0)
    {   // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }

    hh = in.h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }

    out = {out.r * 255, out.g * 255, out.b * 255, out.a * 255};
    return out;     
}

// * KeyboardStates struct method definitions
bool KeyboardState::keydown(SDL_Scancode keycode)
{
    keystates = SDL_GetKeyboardState(NULL); // Update keyboard states
    static bool prev_keystate[SDL_NUM_SCANCODES] = {0}; // Track previous key states
    bool curr_state = keystates[keycode];

    // Check if the key is currently pressed and wasn't pressed in the previous frame
    bool keydown = curr_state && !prev_keystate[keycode];

    prev_keystate[keycode] = curr_state; // Update the previous state for the next frame

    return keydown;
}

bool KeyboardState::keyup(SDL_Scancode keycode)
{
    keystates = SDL_GetKeyboardState(NULL); // Update keyboard states
    static bool prev_keystate[SDL_NUM_SCANCODES] = {0}; // Track previous key states
    bool curr_state = keystates[keycode];

    // Check if the key is currently pressed and wasn't pressed in the previous frame
    bool keyup = !curr_state && prev_keystate[keycode];

    prev_keystate[keycode] = curr_state; // Update the previous state for the next frame

    return keyup;
}

// * SDL functions
void initialize_SDL()
{
    SDL_Init(SDL_INIT_VIDEO);
    display.background_colour = {0, 0, 0, 255};

    // Get display info
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    display.width = display_mode.w;
    display.height = display_mode.h;

    // Create renderer, window, and trail texture
    window = SDL_CreateWindow("Spirograph", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display.width, display.height, SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    trail_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, display.width, display.height);

    return;
}

void quit_SDL()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}

void handleEvents(bool *running, enum Mode *mode, Spirograph *root)
{
    SDL_Event event;
    SDL_GetMouseState(&MouseState.pos.x, &MouseState.pos.y);

    // Settings that should be reset
    MouseState.left_up = MouseState.right_up = MouseState.scroll_up = MouseState.scroll_down = false;
    
    while (SDL_PollEvent(&event))
    {
        // Quit 
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
        {
            *running = 0;
        }

        // Keydown events
        switch (event.type)
        {
            // Keyboard events
            // case SDL_KEYDOWN:
            //     if (event.key.keysym.sym == SDLK_LCTRL && editorState.edit_mode == EditorState::EDIT_MENU)
            //     {   // Switch into new node mode
            //         editorState.edit_mode = EditorState::SET_CHILD_POSITION;
            //     }
            //     break;
            // case SDL_KEYUP:
            //     if (event.key.keysym.sym == SDLK_LCTRL && editorState.edit_mode == EditorState::SET_CHILD_POSITION)
            //     {   // Switch out of new node mode
            //         editorState.edit_mode = EditorState::EDIT_MENU;
            //     }
            //     break;
            
            // Mouse events
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                {   // Left down
                    MouseState.left_down = true;
                    SDL_GetMouseState(&MouseState.left_down_pos.x, &MouseState.left_down_pos.y);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {   // Right down
                    MouseState.right_down = true;
                    SDL_GetMouseState(&MouseState.right_down_pos.x, &MouseState.right_down_pos.y);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT)
                {   // Left up
                    MouseState.left_down = false;
                    MouseState.left_up = true;
                    SDL_GetMouseState(&MouseState.left_up_pos.x, &MouseState.left_up_pos.y);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {   // Right up
                    MouseState.right_down = false;
                    MouseState.right_up = true;
                    SDL_GetMouseState(&MouseState.right_up_pos.x, &MouseState.right_up_pos.y);
                }
                break;
            case SDL_MOUSEWHEEL:
                // Scroll events
                MouseState.scroll_up = (event.wheel.y > 0);
                MouseState.scroll_down = (event.wheel.y < 0);
                break;
        }
    } 
}

void clearRenderer()
{
    SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(display.background_colour));
    SDL_RenderClear(renderer);
    return;
}

// * Draw functions
int SDL_RenderDrawCircle(SDL_Renderer * renderer, int x, int y, int radius)
{
    // Credit: https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius -1;
    status = 0;

    while (offsety >= offsetx) {
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2*offsetx) {
            d -= 2*offsetx + 1;
            offsetx +=1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius)
{
    // Credit: https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius -1;
    status = 0;

    while (offsety >= offsetx)
    {
        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx, x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety, x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety, x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx, x + offsety, y - offsetx);

        if (status < 0) 
        {
            status = -1;
            break;
        }

        if (d >= 2*offsetx)
        {
            d -= 2*offsetx + 1;
            offsetx +=1;
        }
        else if (d < 2 * (radius - offsety))
        {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else
        {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

void drawLine(SDL_Renderer *renderer, RGBA colour, int x0, int y0, int x1, int y1)
{
    // Xiaolin Wu's line algorithm
    // Credit: https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm

    // Helper lambda functions
    auto fpart = [](float x) -> float {return x - (int)x;};
    auto rfpart = [fpart](float x) -> float {return 1 - fpart(x);};
    auto swap = [](int *a, int *b) -> void {
        int temp = *a;
        *a = *b;
        *b = temp;
        return;
    };
    auto plot = [](SDL_Renderer *renderer, int x, int y, RGBA colour) -> void{
        SDL_SetRenderDrawColor(renderer, RGBA_EXPAND(colour));
        SDL_RenderDrawPoint(renderer, x, y);
        return;
    };
    
    int steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    float gradient = (dx == 0.0) ? 1.0 : dy / (float)dx;

    int xend, yend;
    float xgap;
    
    // handle first endpoint
    xend = round(x0);
    yend = y0 + gradient * (xend - x0);
    xgap = rfpart(x0 + 0.5);
    int xpxl1 = xend; // this will be used in the main loop
    int ypxl1 = (int)yend;
    if (steep) {
        plot(renderer, ypxl1    , xpxl1, {colour.r, colour.g, colour.b, 255 * rfpart(yend) * xgap});
        plot(renderer, ypxl1 + 1, xpxl1, {colour.r, colour.g, colour.b, 255 *  fpart(yend) * xgap});
    } else {
        plot(renderer, xpxl1, ypxl1    , {colour.r, colour.g, colour.b, 255 * rfpart(yend) * xgap});
        plot(renderer, xpxl1, ypxl1 + 1, {colour.r, colour.g, colour.b, 255 *  fpart(yend) * xgap});
    }
    float intery = yend + gradient; // first y-intersection for the main loop
    
    // handle second endpoint
    xend = round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5);
    int xpxl2 = xend; //this will be used in the main loop
    int ypxl2 = (int)yend;
    if (steep) {
        plot(renderer, ypxl2    , xpxl2, {colour.r, colour.g, colour.b, 255 * rfpart(yend) * xgap});
        plot(renderer, ypxl2 + 1, xpxl2, {colour.r, colour.g, colour.b, 255 *  fpart(yend) * xgap});
    } else {
        plot(renderer, xpxl2, ypxl2    , {colour.r, colour.g, colour.b, 255 * rfpart(yend) * xgap});
        plot(renderer, xpxl2, ypxl2 + 1, {colour.r, colour.g, colour.b, 255 *  fpart(yend) * xgap});
    }
    
    // main loop
    if (steep) {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
			plot(renderer, (int)intery    , x, {colour.r, colour.g, colour.b, 255 * rfpart(intery)});
			plot(renderer, (int)intery + 1, x, {colour.r, colour.g, colour.b, 255 *  fpart(intery)});
			intery += gradient;
		}
    } else {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
			plot(renderer, x, (int)intery    , {colour.r, colour.g, colour.b, 255 * rfpart(intery)});
			plot(renderer, x, (int)intery + 1, {colour.r, colour.g, colour.b, 255 *  fpart(intery)});
			intery += gradient;
		}
	}

	return;
}
