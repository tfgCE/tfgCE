template <typename CustomType>
DebugVisualiser<CustomType>::DebugVisualiser(Generator<CustomType> * _generator)
: base(String(TXT("piece combiner")))
, generator(_generator)
{
	generatorLock.acquire();
	set_text_scale(0.8f);
}

template <typename CustomType>
DebugVisualiser<CustomType>::~DebugVisualiser()
{
}

template <typename CustomType>
void DebugVisualiser<CustomType>::show(Colour const & _colour, String const & _message)
{
	messageColour = _colour;
	message = _message;

	generatorLock.release();

	bool allowDebug = true;
	if (!markFinished)
	{
		if (Concurrency::ThreadManager::get() && Concurrency::ThreadManager::get_current_thread_id() == 0)
		{
			// for single thread do advance to catch leftctrl+space release
			::System::Core::set_time_multiplier(); 
			::System::Core::advance();
		}
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->is_key_pressed(System::Key::Space) &&
			::System::Input::get()->is_key_pressed(System::Key::LeftCtrl))
		{
			allowDebug = false;
		}
#endif
	}

	if (is_active() && allowDebug)
	{
		allowedNextStep = false;

#ifdef AN_STANDARD_INPUT
		breakKeyInput = System::Key::B; // hardcoded (will auto display too)
#else
		breakKeyInput = System::Key::None;
#endif
		breakKeyPressed = false;
		while (true)
		{
			if (advance_and_render_if_on_render_thread())
			{
				// advance system core
				::System::Core::set_time_multiplier(); 
				::System::Core::advance();
			}
			else
			{
				// sleep as we don't want to stall - advancing and rendering is done somewhere else, on actual render thread?
				::System::Core::sleep(0.1f);
			}
			if (markFinished)
			{
				if (allowedToEnd) break;
			}
			else
			{
				if (allowedNextStep) break;
			}

			if (breakKeyPressed)
			{
				break;
			}
		}
		if (breakKeyPressed)
		{
			AN_BREAK;
		}
	}

	generatorLock.acquire();
}

template <typename CustomType>
void DebugVisualiser<CustomType>::advance()
{
	base::advance();

#ifdef AN_STANDARD_INPUT
	if (!markFinished)
	{
		if (::System::Input::get()->has_key_been_pressed(System::Key::Space))
		{
			// allow one step
			allowedNextStep = true;
		}
		if (::System::Input::get()->is_key_pressed(System::Key::Space) &&
			::System::Input::get()->is_key_pressed(System::Key::LeftShift))
		{
			// allow one step
			allowedNextStep = true;
		}
	}
	else
	{
		if (::System::Input::get()->has_key_been_pressed(System::Key::Return))
		{
			// allow one step
			allowedToEnd = true;
		}
	}
#endif

	// update only if generator is not locked
	if (generatorLock.acquire_if_not_locked())
	{
#ifdef AN_DEVELOPMENT
		displayMessage = message;
		displayMessageColour = messageColour;

		move_things_around();

		start_gathering_data();

		clear();

		add_things_to_display(generator->generationSpace);

		end_gathering_data();

#endif
		generatorLock.release();
	}
}

template <typename CustomType>
void DebugVisualiser<CustomType>::move_things_around()
{
	float deltaTime = ::System::Core::get_delta_time();
	float useNew = clamp(deltaTime / 0.2f, 0.0f, 1.0f);

	// reset moves
	for_every_ptr(piece, generator->access_all_piece_instances())
	{
		piece->dvMove = Vector2::zero;
	}

	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		connector->dvMove = Vector2::zero;
	}

	// do piece separation
	for_every_ptr(pieceA, generator->access_all_piece_instances())
	{
		for_every_ptr(pieceB, generator->access_all_piece_instances())
		{
			if (pieceA != pieceB && pieceA->inGenerationSpace == pieceB->inGenerationSpace)
			{
				Vector2 a2b = pieceB->dvLocation - pieceA->dvLocation;
				float dist = a2b.length();
				float minDist = pieceA->dvRadius + pieceB->dvRadius + minPieceSeparation;
				if (dist == 0.0f)
				{
					a2b.x += Random::get_float(-0.1f, 0.1f);
					a2b.y += Random::get_float(-0.1f, 0.1f);
				}
				if (dist < minDist)
				{
					Vector2 move = a2b.normal() * ((minDist - dist) / minDist) * pieceSeparation_strength;
					pieceA->dvMove -= move;
					pieceB->dvMove += move;
				}
			}
		}
	}

	// do connector-piece/connector separation
	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		if (!connector->get_owner() &&
			(connector->get_connector()->type != ConnectorType::Child &&
			 connector->get_connector()->type != ConnectorType::Internal))
		{
			for_every_ptr(piece, generator->access_all_piece_instances())
			{
				if (connector->inGenerationSpace == piece->inGenerationSpace)
				{
					Vector2 c2p = piece->dvLocation - connector->dvLocation;
					float dist = c2p.length();
					float minDist = connectorRadius + piece->dvRadius + minPieceSeparation;
					float absMinDist = connectorRadius + piece->dvRadius;
					if (dist == 0.0f)
					{
						c2p.x += Random::get_float(-0.1f, 0.1f);
						c2p.y += Random::get_float(-0.1f, 0.1f);
					}
					if (dist < minDist)
					{
						Vector2 move = c2p.normal() * ((minDist - dist) / minDist) * pieceSeparation_strength;
						connector->dvMove -= move;
						piece->dvMove += move;
					}
					if (dist < absMinDist)
					{
						// move out of the piece immediately
						connector->dvLocation -= c2p.normal() * (absMinDist - dist) / absMinDist;
					}
				}
			}
			for_every_ptr(connectorB, generator->access_all_connector_instances())
			{
				if (connector != connectorB &&
					connector->inGenerationSpace == connectorB->inGenerationSpace)
				{
					Vector2 c2b = connectorB->dvLocation - connector->dvLocation;
					float dist = c2b.length();
					float minDist = connectorRadius + connectorRadius + minPieceSeparation;
					if (dist == 0.0f)
					{
						c2b.x += Random::get_float(-0.1f, 0.1f);
						c2b.y += Random::get_float(-0.1f, 0.1f);
					}
					if (dist < minDist)
					{
						Vector2 move = c2b.normal() * ((minDist - dist) / minDist) * connectorSeparation_strength;
						connector->dvMove -= move;
						connectorB->dvMove += move;
					}
				}
			}
		}
	}

	// change angle of connectors
	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		if (connector->connectedTo)
		{
			Vector2 dir = connector->connectedTo->dvLocation - connector->dvLocation;
			float useNew = clamp(deltaTime / 0.7f, 0.0f, 1.0f);
			float newAngle = dir.angle();
			newAngle = connector->dvAngle + relative_angle(newAngle - connector->dvAngle);
			if (connector->owner)
			{
				for_every_ptr(connectorB, generator->access_all_connector_instances())
				{
					if (connector != connectorB &&
						connector->owner == connectorB->owner &&
						connector->inGenerationSpace == connectorB->inGenerationSpace)
					{
						float minAngleSeparation = 20.0f / connector->get_owner()->dvRadius;
						float diffAngle = relative_angle(connectorB->dvAngle - newAngle);
						if (abs(diffAngle) < 0.5f)
						{
							diffAngle += Random::get_float(-1.0f, 1.0f);
						}
						if (abs(diffAngle) < minAngleSeparation)
						{
							newAngle += (minAngleSeparation - abs(diffAngle)) * sign(diffAngle);
						}
					}
				}
			}
			connector->dvAngle = useNew * newAngle + (1.0f - useNew) * connector->dvAngle;
		}
		else if (connector->get_owner())
		{
			float changeAngle = 0.0f;
			float minAngleSeparation = 20.0f / connector->get_owner()->dvRadius;
			for_every_ptr(connectorB, generator->access_all_connector_instances())
			{
				if (connectorB != connector && 
					connectorB->inGenerationSpace == connector->inGenerationSpace &&
					connectorB->get_owner() == connector->get_owner())
				{
					float diffAngle = relative_angle(connectorB->dvAngle - connector->dvAngle);
					if (abs(diffAngle) < 0.5f)
					{
						diffAngle += Random::get_float(-1.0f, 1.0f);
					}
					if (abs(diffAngle) < minAngleSeparation)
					{
						changeAngle -= (minAngleSeparation - abs(diffAngle)) * sign(diffAngle);
					}
				}
			}
			connector->dvAngle = relative_angle(connector->dvAngle + changeAngle);
		}
	}

	// pull connectors towards each other
	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		if (connector->connectedTo)
		{
			Vector2 dir = connector->connectedTo->dvLocation - connector->dvLocation;
			float dist = dir.length();
			dir.normalise();
			if (dist > maxConnectorSeparation)
			{
				dist = min(dist, maxConnectorSeparationCutOff);
				connector->dvMove += dir * (dist - maxConnectorSeparation) * connectorSeparation_strength;
				connector->connectedTo->dvMove -= dir * (dist - maxConnectorSeparation) * connectorSeparation_strength;
			}
		}
	}

	// connector movement to owner movement
	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		if (auto * owner = connector->get_owner())
		{
			if (owner->inGenerationSpace == connector->inGenerationSpace)
			{
				owner->dvMove += connector->dvMove;
				connector->dvMove = Vector2::zero;
			}
		}
	}

	// apply movement
	for_every_ptr(piece, generator->access_all_piece_instances())
	{
		piece->dvLocation += piece->dvMove * useNew;
	}

	for_every_ptr(connector, generator->access_all_connector_instances())
	{
		if (connector->get_connector()->type == ConnectorType::Child ||
			connector->get_connector()->type == ConnectorType::Internal)
		{
			// those will be done in a different manner
			continue;
		}
		connector->dvLocation += connector->dvMove * useNew;
		if (auto * owner = connector->get_owner())
		{
			connector->dvLocation = owner->dvLocation + Vector2(0.0f, owner->dvRadius + connectorRadius).rotated_by_angle(connector->dvAngle);
		}
	}

	// calculate inner radius for pieces (using recurrency)
	update_inner_radius_for_pieces_in(generator->generationSpace);
}

template <typename CustomType>
void DebugVisualiser<CustomType>::update_inner_radius_for_pieces_in(GenerationSpace<CustomType>* _generationSpace)
{
	float deltaTime = ::System::Core::get_delta_time();

	for_every_ptr(piece, _generationSpace->pieces)
	{
		// update radiuses inside
		update_inner_radius_for_pieces_in(piece->generationSpace);

		// find centre point
		Range2 range = Range2::empty;
		for_every_ptr(innerPiece, piece->generationSpace->pieces)
		{
			Range2 r = Range2(Range(innerPiece->dvLocation.x - innerPiece->dvRadius, innerPiece->dvLocation.x + innerPiece->dvRadius),
							  Range(innerPiece->dvLocation.y - innerPiece->dvRadius, innerPiece->dvLocation.y + innerPiece->dvRadius));
			range.include(r);
		}
		for_every_ptr(innerConnector, piece->generationSpace->connectors)
		{
			if (innerConnector->get_connector()->type == ConnectorType::Child ||
				innerConnector->get_connector()->type == ConnectorType::Internal)
			{
				// skip those, they are always on bounds
				continue;
			}
			Range2 r = Range2(Range(innerConnector->dvLocation.x - connectorRadius, innerConnector->dvLocation.x + connectorRadius),
							  Range(innerConnector->dvLocation.y - connectorRadius, innerConnector->dvLocation.y + connectorRadius));
			range.include(r);
		}

		Vector2 centre = range.centre();

		// move all to centre
		Vector2 generationSpaceAt = Vector2::zero;
		Vector2 moveBy = generationSpaceAt - centre;

		float radius = 0.0f;
		for_every_ptr(innerPiece, piece->generationSpace->pieces)
		{
			innerPiece->dvLocation += moveBy;
			float maxDist = (innerPiece->dvLocation - generationSpaceAt).length() + innerPiece->dvRadius;
			radius = max(radius, maxDist);
		}
		for_every_ptr(innerConnector, piece->generationSpace->connectors)
		{
			innerConnector->dvLocation += moveBy;
			if (innerConnector->get_connector()->type == ConnectorType::Child ||
				innerConnector->get_connector()->type == ConnectorType::Internal)
			{
				// skip those, they are always on bounds
				continue;
			}
			float maxDist = (innerConnector->dvLocation - generationSpaceAt).length() + connectorRadius;
			radius = max(radius, maxDist);
		}

		piece->dvInnerRadius = radius == 0.0f? defaultPieceRadius :  radius + minPieceSeparation * 1.5f;
		piece->dvRadius = piece->dvInnerRadius * piece->dvInnerScale;

		// place internal and children connectors
		for_every_ptr(innerConnector, piece->generationSpace->connectors)
		{
			if (innerConnector->get_connector()->type == ConnectorType::Internal &&
				innerConnector->internalExternalConnector)
			{
				innerConnector->dvLocation = (innerConnector->internalExternalConnector->dvLocation - piece->dvLocation); // to point in same dir, we will put it on radius properly soon
				innerConnector->dvLocation = innerConnector->dvLocation.normal() * (piece->dvInnerRadius - connectorRadius);
			}
			else if (innerConnector->get_connector()->type == ConnectorType::Child ||
					 innerConnector->get_connector()->type == ConnectorType::Internal)
			{
				float useNew = clamp(deltaTime / 0.3f, 0.0f, 1.0f);
				Vector2 targetDir = innerConnector->dvLocation;
				if (innerConnector->connectedTo &&
					innerConnector->connectedTo->get_owner())
				{
					targetDir = innerConnector->connectedTo->get_owner()->dvLocation;
				}
				innerConnector->dvLocation = innerConnector->dvLocation * (1.0f - useNew) + useNew * (targetDir.normal() * (piece->dvInnerRadius - connectorRadius));
			}
		}
	}
}

template <typename CustomType>
void DebugVisualiser<CustomType>::add_things_to_display(GenerationSpace<CustomType>* _generationSpace, Vector2 const & _zoomRefPoint, float _zoom)
{
	if (!_generationSpace)
	{
		_generationSpace = generator->generationSpace;
	}
	Vector2 generationSpaceAt = Vector2::zero;
	for_every_ptr(piece, _generationSpace->pieces)
	{
		Vector2 at = piece->dvLocation;
		at = _zoomRefPoint + (at - generationSpaceAt) * _zoom;
		Colour colour = Colour::blue;
		if (piece->visualiseIssue) colour = get_debug_highlight_colour();
		add_circle(at, piece->dvRadius * _zoom, colour);
#ifdef PIECE_COMBINER_DEBUG
		add_text(at, String::printf(TXT("%S [%S>%S]"),
			piece->desc.to_char(),
			piece->connectorTag.to_char(), piece->provideConnectorTag.is_set() ? piece->provideConnectorTag.get().to_char() : TXT("-")
			), colour, _zoom);
#endif
		if (piece->generationSpace)
		{
			add_things_to_display(piece->generationSpace, at, _zoom * piece->dvInnerScale);
		}
	}

	for_every_ptr(connector, _generationSpace->connectors)
	{
		Vector2 at = connector->dvLocation;
		at = _zoomRefPoint + (at - generationSpaceAt) * _zoom;
		Colour colour = Colour::green;
		if (connector->is_optional()) colour = Colour::magenta;
		if (auto * ie = connector->get_internal_external()) if (ie->is_connected()) colour = Colour::green; // it should be not optional, it is required right now
		if (!connector->is_available()) colour = Colour::red;
		if (connector->is_connected()) colour = Colour::blue;
		if (connector->get_connector()->type == ConnectorType::Child) colour = Colour::orange;
		if (connector->get_connector()->type == ConnectorType::Parent) colour = Colour::yellow;
		if (connector->visualiseIssue) colour = get_debug_highlight_colour();
		add_circle(at, connectorRadius * _zoom, colour);
#ifdef PIECE_COMBINER_DEBUG
		if (connector->get_connector()->autoId != NONE)
		{
			add_text(at, String::printf(TXT("%S [%S>%S] :%i: %S/%S -> %S/%S"), connector->get_connector()->name.to_char(),
				connector->connectorTag.to_char(), connector->provideConnectorTag.is_set() ? connector->provideConnectorTag.get().to_char() : TXT("-"),
				connector->get_connector()->autoId,
				connector->get_connector()->linkName.to_char(), connector->get_connector()->linkNameAlt.to_char(),
				connector->get_connector()->linkWith.to_char(), connector->get_connector()->linkWithAlt.to_char()
				), colour, _zoom * 0.25f);
		}
		else
		{
			add_text(at, String::printf(TXT("%S [%S>%S] :: %S/%S -> %S/%S"), connector->get_connector()->name.to_char(),
				connector->connectorTag.to_char(), connector->provideConnectorTag.is_set()? connector->provideConnectorTag.get().to_char() : TXT("-"),
				connector->get_connector()->linkName.to_char(), connector->get_connector()->linkNameAlt.to_char(),
				connector->get_connector()->linkWith.to_char(), connector->get_connector()->linkWithAlt.to_char()
				), colour, _zoom * 0.25f);
		}
#endif
		if (connector->connectedTo)
		{
			Vector2 oAt = connector->connectedTo->dvLocation;
			oAt = _zoomRefPoint + (oAt - generationSpaceAt) * _zoom;
			add_line(at + (oAt - at).normal() * connectorRadius * _zoom, (oAt + at) * 0.5f, colour);
		}
	}
}

template <typename CustomType>
void DebugVisualiser<CustomType>::render()
{
	base::render();

	if (auto* font = get_font())
	{
		::System::Video3D* v3d = ::System::Video3D::get();

		float messageScale = 0.25f / get_use_view_size().y;
		float yd = get_font()->get_height() * messageScale;
		Vector2 at = Vector2::zero;
		at.y = get_use_view_size().y - yd;
		font->draw_text_at_with_border(v3d, displayMessage, displayMessageColour, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2::zero, false);
		at.y -= yd;
		at.y -= yd;

		if (markFinished)
		{
			font->draw_text_at_with_border(v3d, String(TXT("[enter] end")), Colour::white, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2::zero, false);
			at.y -= yd;
		}
		else
		{
			font->draw_text_at_with_border(v3d, String(TXT("[space] next step")), Colour::white, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2::zero, false);
			at.y -= yd;
#ifdef AN_STANDARD_INPUT
			font->draw_text_at_with_border(v3d, String(TXT("[+lsft] step per frame")), ::System::Input::get()->is_key_pressed(System::Key::LeftShift)?
				(::System::Input::get()->is_key_pressed(System::Key::Space) ? Colour::green : Colour::cyan)
				: Colour::white, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2::zero, false);
			at.y -= yd;
			font->draw_text_at_with_border(v3d, String(TXT("[+lctl] don't even show")), ::System::Input::get()->is_key_pressed(System::Key::LeftCtrl) ? 
				(::System::Input::get()->is_key_pressed(System::Key::Space) ? Colour::green : Colour::cyan)
				: Colour::white, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2::zero, false);
			at.y -= yd;
#endif
		}
	}
}

template <typename CustomType>
void DebugVisualiser<CustomType>::use_font(::System::Font* _font, float _fontHeight)
{
	if (_fontHeight == 0.0f)
	{
		_fontHeight = 0.3f;
	}
	base::use_font(_font, _fontHeight);
}
