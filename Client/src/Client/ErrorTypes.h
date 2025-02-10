#pragma once

namespace Client
{
	// Enum of login error types
	enum class LoginErrorType
	{
		None = 0,
		NotFilled, // Not filled inputs
		Incorrect, // Incorrect input
	};

	// Enum of register error types
	enum class RegisterErrorType
	{
		None = 0,
		NotFilled, /// Not filled inputs
		NotMatching, // Not matching passwords
		WrongFormat, // Email is in wrong format
		Existing, // Existing email
	};

	// Enum of invite states
	enum class InviteState
	{
		None = 0,
		NotExisting, // User does not exist
		CannotInviteYourself, // Cannot invite yourself into you team
		AlreadyInvited, // User has already been invited
		AlreadyInTeam, // User is already in the team
		InviteSuccessful, // Invite has been successfully sent
	};

	// Enum of change username states
	enum class ChangeUsernameState
	{
		None = 0,
		Error, // An error occured
		NotFilled, // Not filled input
		ChangeSuccessful, // Change was successfull
	};

	// Enum of change password states
	enum class ChangePasswordState
	{
		None = 0,
		Error, // An error occured
		NotFilled, // Not filled inputs
		NotMatching, // Not matching passwords
		ChangeSuccessful, // Change was successfull
	};
}